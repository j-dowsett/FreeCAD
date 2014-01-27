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

    filepath = new char[30];
}


DlgTemplateSelection::~DlgTemplateSelection()
{
  // no need to delete child widgets, Qt does it all for us
}



void DlgTemplateSelection::addSeries(QString path)
{
    QDir dir(path);
    dir.setFilter(QDir::Dirs);
    QString this_series;

    for (int i=0; i<dir.count(); i++ )
    {
        this_series = dir[i];
        if(series.find(this_series) == series.end())
        {
            series[this_series] = new QTreeWidgetItem(QStringList(this_series));
            ui->templates->addTopLevelItem(series[this_series]);
        }
        addTemplates(dir.absolutePath(), series[this_series]);
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
}

const char * DlgTemplateSelection::getPath()
{
    return filepath;
}

#include "moc_DlgTemplateSelection.cpp"
