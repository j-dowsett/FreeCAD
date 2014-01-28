/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#include <App/Document.h>
#include <App/PropertyStandard.h>

#include "DlgTemplateSelection.h"
#include "ui_DlgTemplateSelection.h"

#include <Gui/Application.h>
#include <QDir>

using namespace DrawingGui;

/* TRANSLATOR DrawingGui::DlgTemplateSelection */

DlgTemplateSelection::DlgTemplateSelection(QWidget* parent, Qt::WFlags fl )
  : QDialog( parent, fl ), ui(new Ui_DlgTemplateSelection)
{
    ui->setupUi(this);
    fillList();
}


DlgTemplateSelection::~DlgTemplateSelection()
{
}


void DlgTemplateSelection::addTemplates(QString path, QTreeWidgetItem * parent)
{
    std::string temp = path.toStdString();

    QDir dir(path, QString::fromAscii("*.svg"));

    for (unsigned int i=0; i<dir.count(); i++ )
    {
        QRegExp rx(QString::fromAscii("(A|B|C|D|E)(\\d)?_(Landscape|Portrait)(_)?(.*).svg"));
        if (rx.indexIn(dir[i]) > -1)
        {
            QString paper = rx.cap(1);
            QString id = rx.cap(2);
            QString orientation = rx.cap(3);
            QString additional = rx.cap(5);
            additional.replace(QString::fromUtf8("_"), QString::fromUtf8(" "));

            QStringList template_details;
            template_details << QString() << paper + id << orientation << additional;
            QTreeWidgetItem * template_item = new QTreeWidgetItem(template_details);
            template_item->setToolTip(1, dir.absoluteFilePath(dir[i]));
            parent->addChild(template_item);
        }
    }
}


void DlgTemplateSelection::addSeries(QString path)
{
    std::string temp_string;

    QDir dir(path);
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QString this_series;

    for (int i=0; i<dir.count(); i++ )
    {
        this_series = dir[i];

        temp_string = this_series.toStdString();

        if(series.find(this_series) == series.end())
        {
            series[this_series] = new QTreeWidgetItem(QStringList(this_series));
            ui->templates->addTopLevelItem(series[this_series]);
        }
        addTemplates(dir.absoluteFilePath(this_series), series[this_series]);
    }
}


void DlgTemplateSelection::fillList()
{
    std::string path = App::Application::getResourceDir();
    path += "Mod/Drawing/Templates/";
    addSeries(QString::fromUtf8(path.c_str()));

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");
    path = hGrp->GetASCII("Templates folder", "");

    if (path != "")
        addSeries(QString::fromUtf8(path.c_str()));

    ui->templates->sortItems(2, Qt::AscendingOrder);
    ui->templates->sortItems(1, Qt::AscendingOrder);
    ui->templates->sortItems(0, Qt::AscendingOrder);
}


QString DlgTemplateSelection::getPath()
{
    return ui->templates->currentItem()->toolTip(1);
}

#include "moc_DlgTemplateSelection.cpp"
