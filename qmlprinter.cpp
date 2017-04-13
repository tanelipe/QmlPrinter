/*
 * Copyright (c) 2014 Taneli Peltoniemi <taneli.peltoniemi@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "qmlprinter.h"

#include <QGraphicsView>

QmlPrinter::QmlPrinter(QObject *parent) :
    QObject(parent)
{
}

QmlPrinter::~QmlPrinter()
{

}

void QmlPrinter::changePrinterOrientation(QPrinter& printer, const int &width, const int &height)
{
    if(width > height)
        printer.setOrientation(QPrinter::Landscape);
    else
        printer.setOrientation(QPrinter::Portrait);
}

bool QmlPrinter::printPDF(const QString &location, QList<QQuickItem*> items, bool showPDF)
{
    if(items.length() == 0) {
        return false;
    }
    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(location);
    printer.setFullPage(true);

    // Change the printer orientation based on the first page
    // This needs to be called before the painting is started as it will only take effect
    // after newPage is called (painter.begin() calls this method)
    changePrinterOrientation(printer, items.at(0)->width(), items.at(0)->height());

    QPainter painter;
    // It's possible to fail here for example if the file location does not allow writing
    if(!painter.begin(&printer)) {
        return false;
    }

    for(int i = 0; i < items.length(); ++i) {
        QQuickItem *pageObject = items.at(i);
        // Change the page width/height to match what the printer gives us
        // This way all the components will be resized accordingly
        pageObject->setProperty("width", printer.pageRect().width());
        pageObject->setProperty("height", printer.pageRect().height());

        paintItem(pageObject, pageObject->window(), &painter);

        // We need to lookahead so we can setup the printer orientation for the next
        // item and add a new page to the printer
        int next = i + 1;
        if(next < items.length()) {
            changePrinterOrientation(printer, items.at(next)->width(), items.at(next)->height());
            printer.newPage();
        }
    }

    painter.end();
    if(showPDF) {
        QDesktopServices::openUrl(QUrl("file:///" + location));
    }
    return true;
}

bool QmlPrinter::print(const QPrinterInfo& info, QList<QQuickItem *> items)
{
    if(items.length() == 0)
        return false;

    QPrinter printer(info);
    //printer.setFullPage(true);

    // Change the printer orientation based on the first page
    // This needs to be called before the painting is started as it will only take effect
    // after newPage is called (painter.begin() calls this method)
    changePrinterOrientation(printer, items.at(0)->width(), items.at(0)->height());

    QPainter painter;

    if(!painter.begin(&printer)) {
        return false;
    }
    for(int i = 0; i < items.length(); ++i) {
        QQuickItem *pageObject = items.at(i);

        // Change the page width/height to match what the printer gives us
        // This way all the components will be resized accordingly

        int width = printer.pageRect().width();
        int height = printer.pageRect().height();

        /*
        width -= (printer.pageLayout().margins().right() * 2);
        height -= (printer.pageLayout().margins().bottom() * 2);*/

        pageObject->setProperty("width", width);
        pageObject->setProperty("height", height);

        paintItem(pageObject, pageObject->window(), &painter);

        // We need to lookahead so we can setup the printer orientation for the next
        // item and add a new page to the printer
        int next = i + 1;
        if(next < items.length()) {
            changePrinterOrientation(printer, items.at(next)->width(), items.at(next)->height());
            printer.newPage();
        }
    }
    painter.end();
    return true;
}

void QmlPrinter::paintItem(QQuickItem *item, QQuickWindow *window, QPainter *painter)
{
    if(!item || !item->isVisible())
        return;

    bool drawChildren = true;

    // This is a bit special case as we need to use childItems instead of children
    if(inherits(item->metaObject(), "QQuickListView")) {
        drawChildren = false;
        QList<QQuickItem*> childItems = item->childItems();
        if(childItems.length() > 0) {
            // First item is the QML ListView
            QQuickItem *listView = childItems.at(0);
            if(listView != nullptr) {
                // Draw the child items of the QML ListView
                QList<QQuickItem*> listViewChildren = listView->childItems();
                foreach(QQuickItem *children, listViewChildren) {
                    paintItem(children, window, painter);
                }
            }
        }
    }
    else if(isCustomPrintItem(item->metaObject()->className())) {
        painter->save();
        if(item->clip()) {
            painter->setClipping(true);
            painter->setClipRect(item->clipRect());
        }

        const int boundingMargin = 5;
        QRectF boundingRect;
        boundingRect.setTop(item->boundingRect().top() - boundingMargin);
        boundingRect.setLeft(item->boundingRect().left() - boundingMargin);
        boundingRect.setHeight(item->boundingRect().height() + boundingMargin * 2);
        boundingRect.setWidth(item->boundingRect().width() + boundingMargin * 2);

        const QRectF rect = item->mapRectToScene(boundingRect);
        QImage image = window->grabWindow();
        painter->drawImage(rect.x(), rect.y(), image, rect.x(), rect.y(), rect.width(), rect.height());
        painter->restore();
        drawChildren = false;
    } else if(item->flags().testFlag(QQuickItem::ItemHasContents)) {
        painter->save();
        if(item->clip()) {
            painter->setClipping(true);
            painter->setClipRect(item->clipRect());
        }
        if(inherits(item->metaObject(), "QQuickRectangle")) {
            paintQQuickRectangle(item, painter);
        } else if(inherits(item->metaObject(), "QQuickText")) {
            paintQQuickText(item, painter);
        } else if(inherits(item->metaObject(), "QQuickImage")) {
            paintQQuickImage(item, painter);
        } else if(inherits(item->metaObject(), "QQuickCanvasItem")) {
            paintQQuickCanvasItem(item, window, painter);
        } else {
            // Fallback to screen capture if we are unable to parse the data
            QRect rect = item->mapRectFromScene(item->boundingRect()).toRect();
            if(window != nullptr) {
                QImage image = window->grabWindow();

                painter->drawImage(rect, image, QRect(rect.x(), rect.y(), rect.width(), rect.height()));
            }
            drawChildren = false;
        }
        painter->restore();
    }
    if(drawChildren) {
        const QObjectList children = item->children();
        foreach(QObject *obj, children) {
            paintItem(qobject_cast<QQuickItem*>(obj), window, painter);
        }
    }
}

void QmlPrinter::paintQQuickCanvasItem(QQuickItem *item, QQuickWindow *window, QPainter *painter)
{
    // No point in continuing as we are unable to grab the image
    if(window == nullptr)
        return;

    const QRectF rect = item->mapRectToScene(item->boundingRect());

    QImage image = window->grabWindow();
    painter->drawImage(rect.x(), rect.y(), image, rect.x(), rect.y(), rect.width(), rect.height());


}

void QmlPrinter::paintQQuickRectangle(QQuickItem *item, QPainter *painter)
{
    const QRect rect = item->mapRectToScene(item->boundingRect()).toRect();
    const QColor color = item->property("color").value<QColor>();
    const QObject* border = item->property("border").value<QObject*>();
    const qreal border_width = border->property("width").value<qreal>();
    const QColor border_color = border->property("color").value<QColor>();
    const qreal radius = item->property("radius").value<qreal>();
    const qreal opacity = item->property("opacity").value<qreal>();

    painter->setBrush(color);
    painter->setOpacity(opacity);

    if(border_width > 0 and not (border_width == 1 and border_color == QColor(Qt::black))) {
        painter->setPen(QPen(border_color, border_width));
    } else {
        painter->setPen(Qt::NoPen);
    }

    if(radius > 0) {
        painter->drawRoundedRect(rect, radius, radius);
    } else {
        painter->drawRect(rect);
    }

}

void QmlPrinter::paintQQuickText(QQuickItem *item, QPainter *painter)
{
    const QRectF rect = item->mapRectToScene(item->boundingRect());
    const QFont font = item->property("font").value<QFont>();
    const QString text = item->property("text").value<QString>();
    const QColor color = item->property("color").value<QColor>();
    const int wrapMode = item->property("wrapMode").value<int>();
    int textFormat = item->property("textFormat").value<int>();
    const int horizontalAlignment = item->property("horizontalAlignment").value<int>();
    const int verticalAlignment   = item->property("verticalAlignment").value<int>();

    const int elide = item->property("elide").value<int>();
    Qt::TextElideMode elideMode = static_cast<Qt::TextElideMode>(elide);

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapMode(wrapMode));
    textOption.setAlignment(static_cast<Qt::Alignment>(horizontalAlignment | verticalAlignment));

    QFontMetrics fm(font);

    if(textFormat == Qt::AutoText) {
        textFormat = Qt::mightBeRichText(text) ? 4 : Qt::PlainText;
    }

    switch (textFormat) {
        case Qt::PlainText: {
            painter->setFont(font);
            painter->setPen(color);

            QRectF textRect = rect;

            if(item->rotation() != 0) {
                int xc = rect.x() + rect.width() / 2;
                int yc = rect.y() + rect.height() / 2;

                QMatrix matrix;
                matrix.translate(xc, yc);
                matrix.rotate(item->rotation());

                painter->setMatrix(matrix);
                painter->setMatrixEnabled(true);

                double PI = 3.14159265358979323846;
                double cosine = cos(static_cast<double>(item->rotation()) * PI / 180.0);
                double sine = sin(static_cast<double>(item->rotation()) * PI / 180.0);

                textRect = QRectF(-abs(sine) * rect.height() * 0.5,
                                  abs(cosine) * rect.width() * 0.5,
                                  rect.height(), rect.width());
            }
            bool fontModified;
            QTextLayout textLayout;
            textLayout.setFont(font);
            textLayout.setTextOption(textOption);
            QTextCharFormat defaultFormat;
            defaultFormat.setForeground(color);

            QList<StyledTextImgTag*> tags;
            StyledText::parse(text, textLayout, tags, QUrl(), qmlContext(item), true, &fontModified, defaultFormat);

            QString elidedText = textLayout.text();
            if(elideMode != Qt::ElideNone) {
                elidedText = fm.elidedText(elidedText, elideMode, item->width());
                textLayout.setText(elidedText);
            }

            textLayout.beginLayout();

            switch(textOption.wrapMode()) {
            case QTextOption::NoWrap:
                textLayout.createLine();
                break;
            case QTextOption::WordWrap:
            case QTextOption::ManualWrap:
            case QTextOption::WrapAnywhere:
            case QTextOption::WrapAtWordBoundaryOrAnywhere: {
                int height = 0;
                forever {
                    QTextLine line = textLayout.createLine();
                    if(!line.isValid())
                        break;
                    line.setLineWidth(item->width());
                    line.setPosition(QPointF(0, height));
                    height += line.height();
                }
            } break;
            default:
                break;
            }

            textLayout.endLayout();
            textLayout.draw(painter, textRect.topLeft());

        } break;
        default:
        case 4: {
            bool fontModified;
            QTextLayout textLayout;
            textLayout.setFont(font);
            textLayout.setTextOption(textOption);
            QTextCharFormat defaultFormat;
            defaultFormat.setForeground(color);

            QList<StyledTextImgTag*> tags;
            StyledText::parse(text, textLayout, tags, QUrl(), qmlContext(item), true, &fontModified, defaultFormat);


            textLayout.beginLayout();
            int height = 0;
            const int leading = 0;
            while (1) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid())
                    break;

                line.setLineWidth(item->width());
                height += leading;
                line.setPosition(QPointF(0, height));
                height += line.height();
            }
            textLayout.endLayout();

            painter->setRenderHint(QPainter::Antialiasing, true);
            textLayout.draw(painter, rect.topLeft());
        } break;
        case Qt::RichText: {
            QRectF textRect = rect;
            if(item->rotation() != 0) {
                int xc = rect.x() + rect.width() / 2;
                int yc = rect.y() + rect.height() / 2;

                QMatrix matrix;
                matrix.translate(xc, yc);
                matrix.rotate(item->rotation());

                painter->setMatrix(matrix);
                painter->setMatrixEnabled(true);

                double PI = 3.14159265358979323846;
                double cosine = cos(static_cast<double>(item->rotation()) * PI / 180.0);
                double sine = sin(static_cast<double>(item->rotation()) * PI / 180.0);

                textRect = QRectF(-abs(sine) * rect.height() * 0.5,
                                  abs(cosine) * rect.width() * 0.5,
                                  rect.height(), rect.width());
            }

            QTextDocument document;
            document.setTextWidth(textRect.width());
            document.setDefaultTextOption(textOption);
            document.setDefaultFont(font);
            document.setHtml(text);

            QAbstractTextDocumentLayout::PaintContext context;
            context.palette.setColor(QPalette::Text, color);

            QAbstractTextDocumentLayout *layout = document.documentLayout();
            painter->translate(textRect.topLeft());
            painter->setRenderHint(QPainter::Antialiasing, true);
            layout->draw(painter, context);
        } break;
    }
}

void QmlPrinter::paintQQuickImage(QQuickItem *item, QPainter *painter)
{
    const QUrl url = item->property("source").value<QUrl>();
    const int fillMode = item->property("fillMode").value<int>();

    QImage image(url.toLocalFile());

    QRect rect = item->mapRectToScene(item->boundingRect()).toRect();
    QRect sourceRect(0, 0, image.width(), image.height());

    switch(fillMode)
    {
        default:
            qWarning() << "QuickItemPainter::paintQuickImage unimplemented fill mode: " << fillMode;
        case 0: // Image.Stretch
            break;
        case 1: { // Image.PreserveAspectFit
            QSize size = sourceRect.size();
            size.scale(rect.width(), rect.height(), Qt::KeepAspectRatio);
        } break;
        case 6: { // Image.Pad
            if(sourceRect.width() > rect.width()) {
                rect.setWidth(sourceRect.width());
            } else {
                sourceRect.setWidth(rect.width());
            }
            if(sourceRect.height() > rect.height()){
                rect.setHeight(sourceRect.height());
            } else {
                sourceRect.setHeight(rect.height());
            }
        } break;
    }
    painter->drawImage(rect, image, sourceRect);
}

bool QmlPrinter::inherits(const QMetaObject *metaObject, const QString &name)
{
    if(metaObject->className() == name) {
        return true;
    } else if(metaObject->superClass()) {
        return inherits(metaObject->superClass(), name);
    }
    return false;
}

void QmlPrinter::addPrintableItem(const QString &item)
{
    printableItems.push_back(item);
}

bool QmlPrinter::isCustomPrintItem(const QString &item)
{
    QListIterator<QString> it(printableItems);
    while(it.hasNext()) {
        QString printableItem = it.next();
        if(item.contains(printableItem))
            return true;
    }
    return false;
}
