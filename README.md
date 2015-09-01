QmlPrinter
==========

Simple Qt component which allows user to print out a QML view from C++. 

Based on the work of Cyrille Berger (https://github.com/cyrilleberger/PrintML)

Installation / Setup
=========
```
Clone the repository:
git clone https://github.com/tanelipe/QmlPrinter/

Edit your projects .pro file to contain the following line

include($$PWD/QmlPrinter/QmlPrinter.pri)
```

Examples
==========
```
// Get the root view object
QQuickItem *root = ...
// Print the view to Report.pdf located in Public Documents and then open it with a default PDF viewer
QmlPrinter printer;
printer.printPDF("C:\\Users\\Public\\Documents\\Report.pdf", root, true);
```

Printing to actual printer
```
// Get the root view object
QQuickItem *root = ...
// Get list of available printer names for the computer
QStringList names = QPrinterInfo::availablePrinterNames();
// Let the user select one
QPrinterInfo info = QPrinterInfo::printerInfo(selectedPrinterName);
// Check that the printer is valid and usable (see QPrinterInfo documents for more information)
QmlPrinter printer;
printer.print(info, qobject_cast<QQuickItem*>(root));
```
