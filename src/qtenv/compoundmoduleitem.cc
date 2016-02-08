//==========================================================================
//  COMPOUNDMODULEITEM.CC - part of
//
//                     OMNeT++/OMNEST
//            Discrete System Simulation in C++
//
//==========================================================================

/*--------------------------------------------------------------*
  Copyright (C) 1992-2015 Andras Varga
  Copyright (C) 2006-2015 OpenSim Ltd.

  This file is distributed WITHOUT ANY WARRANTY. See the file
  `license' for details on this and other legal matters.
*--------------------------------------------------------------*/

#include "compoundmoduleitem.h"
#include "submoduleitem.h"

#include <QPen>
#include <QDebug>
#include <omnetpp/cdisplaystring.h>
#include "qtenv.h"
#include "qtutil.h"

namespace omnetpp {
namespace qtenv {

void CompoundModuleItem::updateRectangle()
{
    rectangle->setRect(area.adjusted(-outlineWidth / 2, -outlineWidth / 2, outlineWidth / 2, outlineWidth / 2));
    rectangle->setBrush(backgroundColor);
    rectangle->setPen(QPen(outlineColor, outlineWidth, Qt::SolidLine, Qt::FlatCap, Qt::MiterJoin));
}

void CompoundModuleItem::updateImage() {
    delete imageItem;
    imageItem = nullptr;
    imageContainer->setBrush(Qt::NoBrush);
    // not including the border outline
    imageContainer->setRect(area);

    if (image) {
        if (imageMode == MODE_TILE) {
            // The tiling mode is a bit of an oddball. In this case there is
            // no pixmap item, but the container item above is drawn with
            // a pixmap brush, using the required image. And that draws tiled.
            imageContainer->setBrush(
                        QPixmap::fromImage(*image) // Must take zooming into account.
                        .scaled(image->width() * zoomFactor, image->height() * zoomFactor,
                                Qt::KeepAspectRatio, Qt::SmoothTransformation));
        } else {
            // This is the image itself, clipped by the container above, and
            // set to size/position as needed. Not used in tiling mode.
            imageItem = new QGraphicsPixmapItem(imageContainer);
            imageItem->setPixmap(QPixmap::fromImage(*image));
            // setting up some defaults
            imageItem->setOffset(-image->width() / 2.0, -image->height() / 2.0);
            imageItem->setPos(area.center());
            imageItem->setScale(zoomFactor);
            imageItem->setTransformationMode(Qt::SmoothTransformation);

            switch (imageMode) {
            case MODE_STRETCH:
                imageItem->setScale(1); // undoing the regular zoom scaling
                imageItem->scale(area.width() / image->width(), // Stretching to correct size.
                                 area.height() / image->height());
                break;
            case MODE_TILE:
                Q_ASSERT(false);
                // Already handled in the other branch above, this should never execute.
                break;
            case MODE_CENTER:
                // the defaults above do exactly this
                break;
            case MODE_FIX: // fixed on the top left corner
                // no positioning at all, please. the zoom scaling can stay.
                imageItem->setOffset(0, 0);
                imageItem->setPos(area.topLeft());
                break;
            default:
                ASSERT2(false, "unhandled image mode");
            }
        }
    }
}

void CompoundModuleItem::updateGrid() {
    for (auto l : gridLines)
        delete l;

    gridLines.clear();

    if (gridMajorDistance <= 0)
        return;

    // XXX this code assumes that the top left corner of the
    // border rectangle is always at (0,0) on the layer
    // which is true until the bgp tag is not supported

    auto majorPen = QPen(gridColor, 0, Qt::SolidLine, Qt::FlatCap);
    auto minorPen = QPen(gridColor, 0, Qt::DashLine, Qt::FlatCap);

    int majorIndex = 1;

    while (true) {
         // the int cast and the half pixel offset is to counteract antialiasing
        double majorPos = (int)(majorIndex * gridMajorDistance * zoomFactor) - 0.5;

        if (majorPos < area.height()) {
            auto horizLine = new QGraphicsLineItem(0, majorPos, area.width(), majorPos, this);
            horizLine->setPen(majorPen);
        }
        if (majorPos < area.width()) {
            auto vertLine = new QGraphicsLineItem(majorPos, 0, majorPos, area.height(), this);
            vertLine->setPen(majorPen);
        }

        if (majorPos >= area.height() && majorPos >= area.width()) {
            break;
        }

        ++majorIndex;
    }

    if (gridMinorNum <= 1)
        return;

    int minorIndex = 1;
    while (true) {
        // just avoiding drawing a minor line over a major line
        // (otherwise if the antialiasing smears one of them, it might look ugly)
        if (minorIndex % gridMinorNum != 0) {
             // the int cast and half pixel offset is to counteract antialiasing
            double minorPos = (int)(minorIndex * gridMajorDistance * zoomFactor / gridMinorNum) - 0.5;

            if (minorPos < area.height()) {
                auto horizLine = new QGraphicsLineItem(0, minorPos, area.width(), minorPos, this);
                horizLine->setPen(minorPen);
            }

            if (minorPos < area.width()) {
                auto vertLine = new QGraphicsLineItem(minorPos, 0, minorPos, area.height(), this);
                vertLine->setPen(minorPen);
            }

            if (minorPos >= area.height() && minorPos >= area.width()) {
                break;
            }
        }

        ++minorIndex;
    }
}

CompoundModuleItem::CompoundModuleItem(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    rectangle = new QGraphicsRectItem(this);
    rectangle->setZValue(-2);

    imageContainer = new QGraphicsRectItem(this);
    imageContainer->setZValue(-1);
    imageContainer->setPen(Qt::NoPen);
    imageContainer->setBrush(Qt::NoBrush);
    // this makes it clip the child image
    imageContainer->setFlag(QGraphicsItem::ItemClipsChildrenToShape);

    // grid lines will be at Z = 0

    modulePath = new OutlinedTextItem(this);
    // just moving it off the left border outline
    modulePath->setPos(area.topLeft() + QPointF(2, 0));
    modulePath->setZValue(1);

    // text items will be at Z = 2
}

CompoundModuleItem::~CompoundModuleItem() {
    for (auto l : gridLines)
        delete l;
    delete rectangle;
    delete imageItem;
    delete imageContainer;
    delete modulePath;
}

void CompoundModuleItem::setZoomFactor(double zoom) {
    if (zoomFactor != zoom) {
        zoomFactor = zoom;
        updateRectangle();
        updateImage();
        updateGrid();
    }
}

void CompoundModuleItem::setArea(QRectF area) {
    if (this->area != area) {
        this->area = area;
        updateRectangle();
        updateImage();
        updateGrid();
        modulePath->setPos(area.topLeft() + QPointF(2, 0));
    }
}

QRectF CompoundModuleItem::getArea() {
    return area;
}

void CompoundModuleItem::setBackgroundColor(const QColor &color) {
    if (backgroundColor != color) {
        backgroundColor = color;
        updateRectangle();
    }
}

void CompoundModuleItem::setOutlineColor(const QColor &color) {
    if (outlineColor != color) {
        outlineColor = color;
        updateRectangle();
    }
}

void CompoundModuleItem::setOutlineWidth(double width) {
    if (outlineWidth != width) {
        outlineWidth = width;
        updateRectangle();
    }
}

void CompoundModuleItem::setImage(QImage *image) {
    if (this->image != image) {
        this->image = image;
        updateImage();
    }
}

void CompoundModuleItem::setImageMode(ImageMode mode) {
    if (this->imageMode != mode) {
        this->imageMode = mode;
        updateImage();
    }
}

void CompoundModuleItem::setGridMajorDistance(double gridDistance) {
    if (gridMajorDistance != gridDistance) {
        gridMajorDistance = gridDistance;
        updateGrid();
    }
}

void CompoundModuleItem::setGridMinorNum(int minorNum) {
    if (gridMinorNum != minorNum) {
        gridMinorNum = minorNum;
        updateGrid();
    }
}

void CompoundModuleItem::setGridColor(const QColor &color) {
    if (gridColor != color) {
        gridColor = color;
        updateGrid();
    }
}

void CompoundModuleItem::setModulePath(const QString &path) {
    modulePath->setText(path);
}

int CompoundModuleItem::addText(const CompoundModuleItem::TextData &data) {
    auto item = new OutlinedTextItem(this);
    item->setPos(data.position);
    item->setText(data.text);
    item->setBrush(data.color);
    item->setZValue(2);

    texts.append(data);
    textItems.append(item);

    return texts.length() - 1;
}

int CompoundModuleItem::addText(const QPointF &pos, const QString &text, const QColor &color) {
    return addText(TextData{pos, text, color});
}


void CompoundModuleItemUtil::setupFromDisplayString(CompoundModuleItem *cmi, cModule *mod, double zoomFactor, QRectF submodulesRect)
{
    cDisplayString ds = mod->hasDisplayString() && mod->parametersFinalized()
            ? mod->getDisplayString()
            : cDisplayString();

    // replacing $param args with the actual parameter values
    std::string buffer;
    ds = substituteDisplayStringParamRefs(ds, buffer, mod, true);

    QRectF border = submodulesRect;

    if (border.top() < 0) border.setTop(0);
    if (border.left() < 0) border.setLeft(0);

    // making the margin symmetric
    auto top = border.top();
    if (top >= 0) {
        border.setTop(0);
        border.setBottom(border.bottom() + top);
    }

    auto left = border.left();
    if (left >= 0) {
        border.setLeft(0);
        border.setRight(border.right() + left);
    }

    bool widthOk, heightOk;
    double width, height;
    width = QString(ds.getTagArg("bgb", 0)).toDouble(&widthOk) * zoomFactor;
    height = QString(ds.getTagArg("bgb", 1)).toDouble(&heightOk) * zoomFactor;

    if (widthOk) border.setWidth(width);
    if (heightOk) border.setHeight(height);

    cmi->setZoomFactor(zoomFactor);
    cmi->setArea(border);

    cmi->setBackgroundColor(parseColor(ds.getTagArg("bgb", 2), parseColor("grey82")));
    cmi->setOutlineColor(parseColor(ds.getTagArg("bgb", 3), QColor("black")));

    double outlineWidth;
    bool ok;
    outlineWidth = QString(ds.getTagArg("bgb", 4)).toDouble(&ok);
    if (!ok) outlineWidth = 2;
    cmi->setOutlineWidth(outlineWidth);

    cmi->setData(1, QVariant::fromValue((cObject*)mod));

    // background tooltip
    cmi->setToolTip(ds.getTagArg("bgtt", 0));

    // background image
    const char *imageName = ds.getTagArg("bgi", 0);
    const char *imageMode = ds.getTagArg("bgi", 1);

    cmi->setImage(imageName[0] ? getQtenv()->icons.getImage(imageName) : nullptr);
    switch (imageMode[0]) {
    case 's': cmi->setImageMode(CompoundModuleItem::MODE_STRETCH); break;
    case 'c': cmi->setImageMode(CompoundModuleItem::MODE_CENTER);  break;
    case 't': cmi->setImageMode(CompoundModuleItem::MODE_TILE);    break;
    default:  cmi->setImageMode(CompoundModuleItem::MODE_FIX);     break;
    }

    // grid
    // if failed to parse, the default 0 will disable the grid
    cmi->setGridMajorDistance(QString(ds.getTagArg("bgg", 0)).toDouble());
    // if failed to parse, the default 0 will disable the minor lines
    cmi->setGridMinorNum(QString(ds.getTagArg("bgg", 1)).toInt());
    cmi->setGridColor(parseColor(ds.getTagArg("bgg", 2), QColor("grey")));

    // the text in the top left corner
    cmi->setModulePath(mod->getFullPath().c_str());

    // background texts
    int textIndex = 0;
    while (true) {
        // the first indexed is bgt1
        std::string tagName = (QString("bgt") + ((textIndex > 0) ? QString::number(textIndex) : "")).toStdString();
        if (!ds.containsTag(tagName.c_str()))
            break;

        cmi->addText(QPointF(
                         QString(ds.getTagArg(tagName.c_str(), 0)).toDouble(),
                         QString(ds.getTagArg(tagName.c_str(), 1)).toDouble()),
                     ds.getTagArg(tagName.c_str(), 2),
                     parseColor(ds.getTagArg(tagName.c_str(), 3), QColor("black")));

        ++textIndex;
    }
}


QRectF CompoundModuleItem::boundingRect() const {
    return area;
}

void CompoundModuleItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // nothing to do here, the children do the painting
}


} // namespace qtenv
} // namespace omnetpp
