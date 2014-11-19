/*
 * Copyright (c) 2014 Taneli Peltoniemi <taneli.peltoniemi@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "qmlprinter.h"

QmlPrinter::QmlPrinter(QObject *parent) :
    QObject(parent)
{
}

QmlPrinter::~QmlPrinter()
{

}

void QmlPrinter::printPDF(const QString &location, QQuickItem *item, bool showPDF)
{
    if(!item)
        return;

    QPrinter printer;
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(location);
    printer.setFullPage(true);
    if(item->width() > item->height())
        printer.setOrientation(QPrinter::Landscape);
    else
        printer.setOrientation(QPrinter::Portrait);
    printer.setPaperSize(QPrinter::A4);

    // Resizes the root object to the resolution that the printer uses for printable area
    item->setProperty("width", printer.pageRect().width());
    item->setProperty("height", printer.pageRect().height());

    QPainter painter;
    painter.begin(&printer);
    paintItem(item, item->window(), &painter);
    painter.end();

    if(showPDF) {
        QDesktopServices::openUrl(QUrl("file:///" + location));
    }
}

void QmlPrinter::paintItem(QQuickItem *item, QQuickWindow *window, QPainter *painter)
{
    if(!item || !item->isVisible())
        return;

    if(item->flags().testFlag(QQuickItem::ItemHasContents)) {
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
        }
        painter->restore();
    }

    const QObjectList children = item->children();
    foreach(QObject *obj, children) {
        paintItem(qobject_cast<QQuickItem*>(obj), window, painter);
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

    painter->setBrush(color);

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

    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapMode(wrapMode));
    textOption.setAlignment((Qt::Alignment)(horizontalAlignment | verticalAlignment));



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
                double cosine = cos((double)item->rotation() * PI / 180.0);
                double sine = sin((double)item->rotation() * PI / 180.0);

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

            textLayout.beginLayout();
            int height = 0;
            const int leading = 0;
            while (true) {
                QTextLine line = textLayout.createLine();
                if (!line.isValid())
                    break;

                line.setLineWidth(item->width());
                height += leading;
                line.setPosition(QPointF(0, height));
                height += line.height();
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
            QTextDocument doc;
            doc.setTextWidth(rect.width());
            doc.setDefaultTextOption(textOption);
            doc.setDefaultFont(font);

            doc.setHtml(text);

            QAbstractTextDocumentLayout::PaintContext context;

            context.palette.setColor(QPalette::Text, color);

            QAbstractTextDocumentLayout *layout = doc.documentLayout();
            painter->translate(rect.topLeft());
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

    QRect rect = item->mapRectToScene(item->boundingRect()).toRect(); // TODO should it be set on the QPainter ?
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
