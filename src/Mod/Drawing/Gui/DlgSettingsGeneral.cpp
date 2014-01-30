/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Interface_Static.hxx>

#include <Base/Parameter.h>
#include <App/Application.h>

#include "DlgSettingsGeneral.h"
#include "ui_DlgSettingsGeneral.h"


using namespace DrawingGui;

DlgSettingsGeneral::DlgSettingsGeneral(QWidget* parent)
  : PreferencePage(parent)
{
    ui = new Ui_DlgSettingsGeneral();
    ui->setupUi(this);

    ui->templatePath->setMode(Gui::FileChooser::Directory);
    findSeries();       // populates drop down box for default template series

    connect(ui->templatePath, SIGNAL(fileNameChanged(QString)), this, SLOT(updateTemplatePath(QString)));       // need to catch this to update default series combobox
}


DlgSettingsGeneral::~DlgSettingsGeneral()
{
    delete ui;
}


void DlgSettingsGeneral::updateTemplatePath(QString junk)
{
    ui->templatePath->onSave();
    ui->defaultSeries->clear();
    findSeries();
    selectSeries();
}


void DlgSettingsGeneral::findSeries()
{
    QStringList series;

    std::string path = App::Application::getResourceDir();
    path += "Mod/Drawing/Templates/";
    QDir dir(QString::fromUtf8(path.c_str()));
    dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

    for (int i=0; i<dir.count(); i++ )
        series << dir[i];


    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");
    path = hGrp->GetASCII("Templates folder", "");

    if (path != "")
    {
        dir.setPath(QString::fromUtf8(path.c_str()));
        dir.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);

        for (int i=0; i<dir.count(); i++ )
            series << dir[i];
    }

    series.removeDuplicates();
    series.sort();
    ui->defaultSeries->insertItems(0, series);
}


void DlgSettingsGeneral::selectSeries()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");
    std::string series = hGrp->GetASCII("Default series", "");
    int i;

    for (i = ui->defaultSeries->count() - 1; i > 0; i--)
    {
        if (ui->defaultSeries->itemText(i).toStdString() == series)
            break;
    }

    ui->defaultSeries->setCurrentIndex(i);          // if preference doesn't exist (or series can't be found), will select top item in list
}


void DlgSettingsGeneral::saveSettings()
{
    ui->orthoSpace->onSave();
    ui->defaultProj->onSave();
    ui->templatePath->onSave();

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");
    hGrp->SetASCII("Default series", ui->defaultSeries->currentText().toStdString().c_str());
}


void DlgSettingsGeneral::loadSettings()
{
    ui->orthoSpace->onRestore();
    ui->defaultProj->onRestore();
    ui->templatePath->onRestore();
    selectSeries();
}


void DlgSettingsGeneral::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}
#include "moc_DlgSettingsGeneral.cpp"

