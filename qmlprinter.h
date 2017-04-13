/*
 * Copyright (c) 2014 Taneli Peltoniemi <taneli.peltoniemi@gmail.com>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
#ifndef QMLPRINTER_H
#define QMLPRINTER_H

#include <QObject>
#include <QQuickItem>
#include <QQuickWindow>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QDesktopServices>
#include <QAbstractTextDocumentLayout>
#include <QTextDocument>
#include "styledtext.h"
#include <QPrinterInfo>
class QmlPrinter : public QObject
{
    Q_OBJECT
private:

    QList<QString> printableItems;

    void paintItem(QQuickItem *item, QQuickWindow *window, QPainter *painter);
    void paintQQuickRectangle(QQuickItem *item, QPainter *painter);
    void paintQQuickText(QQuickItem *item, QPainter *painter);
    void paintQQuickImage(QQuickItem *item, QPainter *painter);
    void paintQQuickCanvasItem(QQuickItem *item, QQuickWindow *window, QPainter *painter);

    bool inherits(const QMetaObject *metaObject, const QString &name);
    bool isCustomPrintItem(const QString &item);

    void changePrinterOrientation(QPrinter& printer, const int& width, const int& height);
public:
    explicit QmlPrinter(QObject *parent = 0);
    virtual ~QmlPrinter();

    bool printPDF(const QString &location, QList<QQuickItem *> items, bool showPDF = false);
    bool print(const QPrinterInfo& info, QList<QQuickItem*> items);
    void addPrintableItem(const QString &item);
signals:

public slots:

};

#endif // HURPRINTER_H
