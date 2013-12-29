/***************************************************************************
 *   Copyright (c) 2012 Joe Dowsett <j-dowsett[at]users.sourceforge.net>   *
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

#ifndef _PreComp_
# include <QMessageBox>
#endif

#include "TaskOrthoViews.h"
#include "ui_TaskOrthoViews.h"

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Base/BoundBoxPy.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Drawing/App/FeaturePage.h>

//#include <Base/FileInfo.h>
//#include <iostream>
//#include <Standard_Failure.hxx>

using namespace Gui;
using namespace DrawingGui;
using namespace std;


#ifndef PI
    #define PI    3.14159265358979323846 /* pi */
#endif



void pagesize(string & page_template, int & x, int & y)
{
    x = 297;    // default to A4 landscape simple
    y = 210;

    /********update num_templates when adding extra templates*******************/
    const int num_templates = 2;
    string templates[num_templates] = {"A3_Landscape.svg", "A4_Landscape.svg"};
    int dimensions[num_templates][2] = {{420, 237}, {297, 163}};       //width, height  measured from page edge, upto info box

    for (int i=0; i < num_templates; i++)
    {
        if (page_template.find(templates[i]) != string::npos)
        {
            x = dimensions[i][0];
            y = dimensions[i][1];
            return;
        }
    }

    //matching template not found, read through template file for width & height info.

    //code below copied from FeaturePage.cpp
    Base::FileInfo fi(page_template);
    if (!fi.isReadable())
    {
        fi.setFile(App::Application::getResourceDir() + "Mod/Drawing/Templates/" + fi.fileName());
        if (!fi.isReadable())       //if so then really shouldn't have been able to get this far, but just in case...
            return;
    }

    // open Template file
    string line;
    string temp_line;
    ifstream file (fi.filePath().c_str());
    size_t found;

    try
    {
        while (!file.eof())
        {
            getline (file,line);
            found = line.find("width=");
            if (found != string::npos)
            {
                temp_line = line.substr(7 + found);
                sscanf (temp_line.c_str(), "%i", &x);
            }

            found = line.find("height=");
            if (found != string::npos)
            {
                temp_line = line.substr(8+found);
                sscanf (temp_line.c_str(), "%i", &y);
            }

            if (line.find("metadata") != string::npos)      //give up if we meet a metadata tag
                break;
        }
    }
    catch (Standard_Failure)
    { }

    file.close();
}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////




orthoView::orthoView(string name, string label, const char * targetpage, const char * sourcepart, Base::BoundBox3d * partbox)
{
    myname = name;
    //mybox = partbox;

    cx = partbox->CalcCenter().x;
    cy = partbox->CalcCenter().y;
    cz = partbox->CalcCenter().z;

    pageX = 0;
    pageY = 0;
    scale = 1;

    Command::doCommand(Command::Doc,"App.activeDocument().addObject('Drawing::FeatureViewPart','%s')", myname.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Label = '%s'", myname.c_str(), label.c_str());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Source = App.activeDocument().%s", myname.c_str(), sourcepart);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.addObject(App.activeDocument().%s)", targetpage, myname.c_str());
    // Command::doCommand(Command::Doc,"App.activeDocument().%s.Direction = (1,0,0)", myname.c_str());
}


orthoView::~orthoView()
{
}


void orthoView::deleteme()
{
    Command::doCommand(Command::Doc,"App.activeDocument().removeObject('%s')", myname.c_str());
}


void orthoView::setPos(float px, float py)
{
    if (px != 0 && py !=0)
    {
        pageX = px;
        pageY = py;
    }

    float ox = pageX - scale * x;
    float oy = pageY + scale * y;

    Command::doCommand(Command::Doc,"App.activeDocument().%s.X = %f", myname.c_str(), ox);
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Y = %f", myname.c_str(), oy);
}


void orthoView::setScale(float newScale)
{
    scale = newScale;
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Scale = %f",myname.c_str(), scale);
}


void orthoView::calcCentre()
{
    x = X_dir.X() * cx + X_dir.Y() * cy + X_dir.Z() * cz;
    y = Y_dir.X() * cx + Y_dir.Y() * cy + Y_dir.Z() * cz;
}


void orthoView::hidden(bool state)
{
    if (state)
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowHiddenLines = True", myname.c_str());
    else
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowHiddenLines = False", myname.c_str());
}


void orthoView::smooth(bool state)
{
    if (state)
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowSmoothLines = True", myname.c_str());
    else
        Command::doCommand(Command::Doc,"App.activeDocument().%s.ShowSmoothLines = False", myname.c_str());
}


void orthoView::set_projection(gp_Ax2 cs)
{
    gp_Ax2  actual_cs;
    gp_Dir  actual_X;
    float   tx, ty, tz;

    // coord system & directions for desired projection
    X_dir = cs.XDirection();
    Y_dir = cs.YDirection();
    Z_dir = cs.Direction();

    // need to get exact same values that will be given to the projection algos.
    char temp[10];
    sprintf(temp, "%f", Z_dir.X());
    tx = atof(temp);
    sprintf(temp, "%f", Z_dir.Y());
    ty = atof(temp);
    sprintf(temp, "%f", Z_dir.Z());
    tz = atof(temp);

    // coord system of created view - same code as used in projection algos
    actual_cs = gp_Ax2(gp_Pnt(0,0,0), gp_Dir(tx,ty,tz));
    actual_X = actual_cs.XDirection();

    // angle between desired projection and actual projection
    float rotation = X_dir.Angle(actual_X);

    if (rotation != 0 && abs(PI - rotation) > 0.05)
        if (!Z_dir.IsEqual(actual_X.Crossed(X_dir), 0.05))
            rotation = -rotation;

    calcCentre();

    Command::doCommand(Command::Doc,"App.activeDocument().%s.Direction = (%f,%f,%f)", myname.c_str(), Z_dir.X(), Z_dir.Y(), Z_dir.Z());
    Command::doCommand(Command::Doc,"App.activeDocument().%s.Rotation = %f", myname.c_str(), 180 * rotation / PI);
}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////




OrthoViews::OrthoViews(const char * pagename, const char * partname)
{
    page_name = pagename;
    part_name = partname;

    parent_doc = App::GetApplication().getActiveDocument();

    App::DocumentObject * part = parent_doc->getObject(partname);
    bbox.Add(static_cast<Part::Feature*>(part)->Shape.getBoundingBox());

    App::DocumentObject * page = parent_doc->getObject(pagename);
    string template_name = static_cast<Drawing::FeaturePage*>(page)->Template.getValue();

    pagesize(template_name, size_x, size_y);

    margin = 10;                // be nice to get from the template itself
    min_space = 15;             // should be preferenced
    size_x -= 2 * margin;
    size_y -= 2 * margin;

    min_r_x = max_r_x = 0;
    min_r_y = max_r_y = 0;

    rotate_coeff = 1;
    smooth = false;
    hidden = false;
    autodims = true;
}


OrthoViews::~OrthoViews()
{
    for (int i = views.size() - 1; i >= 0; i--)          // start from 1 - the 0 is the primary view
        delete views[i].view;
}


void OrthoViews::set_hidden(bool state)
{
    hidden = state;

    for (int i = 0; i < views.size(); i++)
        views[i].view->hidden(hidden);

    Command::updateActive();
    Command::commitCommand();
}


void OrthoViews::set_smooth(bool state)
{
    smooth = state;

    for (int i = 0; i < views.size(); i++)
        views[i].view->smooth(smooth);

    Command::updateActive();
    Command::commitCommand();
}


void OrthoViews::set_primary(gp_Dir facing, gp_Dir right)   // set the orientation of the primary view
{
    primary.SetDirection(facing);
    primary.SetXDirection(right);
    gp_Dir up = primary.YDirection();

    // compute dimensions of part when orientated according to primary view
    width = abs(right.X() * bbox.LengthX() + right.Y() * bbox.LengthY() + right.Z() * bbox.LengthZ());
    height = abs(up.X() * bbox.LengthX() + up.Y() * bbox.LengthY() + up.Z() * bbox.LengthZ());
    depth = abs(facing.X() * bbox.LengthX() + facing.Y() * bbox.LengthY() + facing.Z() * bbox.LengthZ());

    if (views.size() == 0)
        add_view(0, 0);
    else
    {
        views[0].view->set_projection(primary);
        set_all_orientations();                 // reorient all other views appropriately
        process_views();
    }
}


void OrthoViews::calc_layout_size()                         // calculate the real world size of given view layout, assuming no space
{
    // note that views in relative positions x = -4, -2, 0 , 2 etc etc
    // have width = orientated part width
    // while those in relative positions x = -3, -1, 1 etc
    // have width = orientated part depth

    // similarly in y positions, height = part height / depth

    layout_width = (1 + floor(max_r_x / 2.0) + floor(-min_r_x / 2.0)) * width;
    layout_width += (ceil(max_r_x / 2.0) + ceil(-min_r_x / 2.0)) * depth;
    layout_height = (1 + floor(max_r_y / 2.0) + floor(-min_r_y / 2.0)) * height;
    layout_height += (ceil(max_r_y / 2.0) + ceil(-min_r_y / 2.0)) * depth;
}


void OrthoViews::set_projection(int proj)                   // 1 = 1st angle, 3 = 3rd angle
{
    if (proj == 3)
        rotate_coeff = 1;
    else if (proj == 1)
        rotate_coeff = -1;

    set_all_orientations();
    process_views();
}


void OrthoViews::set_all_orientations()                     // set orientations of all views (ie projection or primary changed)
{
    for (int i = 1; i < views.size(); i++)          // start from 1 - the 0 is the primary view
    {
        if (views[i].ortho)
            set_orientation(i);
        else
            set_Axo(views[i].rel_x, views[i].rel_y);
    }
}


void OrthoViews::set_orientation(int index)                 // set orientation of single view
{
    double  rotation;
    int     n;                                              // how many 90* rotations from primary view?
    gp_Dir  dir;                                            // rotate about primary x axis (if in a relative y position) or y axis?
    gp_Ax2  cs;

    if (views[index].ortho)
    {
        if (views[index].rel_x != 0)
        {
            dir = primary.YDirection();
            n = views[index].rel_x;
        }
        else
        {
            dir = primary.XDirection();
            n = -views[index].rel_y;
        }

        rotation = n * rotate_coeff * M_PI_2l;              // rotate_coeff is -1 or 1 for 1st or 3rd angle
        cs = primary.Rotated(gp_Ax1(gp_Pnt(0,0,0), dir), rotation);
        views[index].view->set_projection(cs);
    }
}


void OrthoViews::set_views()                                // process all views - scale & positions
{
    float x;
    float y;

    for (int i = 0; i < views.size(); i++)
    {
        x = offset_x + (views[i].rel_x - min_r_x) * gap_x;
        y = offset_y + (max_r_y - views[i].rel_y) * gap_y;

        if (views[i].auto_scale)
            views[i].view->setScale(scale);
        views[i].view->setPos(x, y);
    }
}


void OrthoViews::calc_offsets()                             // calcs SVG coords for centre of upper left view
{
    // space_x is the emptry clear white space between views
    // gap_x is the centre - centre distance between views

    float space_x = (size_x - scale * layout_width) / num_gaps_x;
    float space_y = (size_y - scale * layout_height) / num_gaps_y;

    gap_x = space_x + scale * (width + depth) * 0.5;
    gap_y = space_y + scale * (height + depth) * 0.5;

    if (min_r_x % 2 == 0)
        offset_x = margin + space_x + 0.5 * scale * width;
    else
        offset_x = margin + space_x + 0.5 * scale * depth;

    if (max_r_y % 2 == 0)
        offset_y = margin + space_y + 0.5 * scale * height;
    else
        offset_y = margin + space_y + 0.5 * scale * depth;
}


void OrthoViews::calc_scale()                               // compute scale required to meet minimum space requirements
{
    float scale_x, scale_y, working_scale;

    scale_x = (size_x - num_gaps_x * min_space) / layout_width;
    scale_y = (size_y - num_gaps_y * min_space) / layout_height;

    working_scale = min(scale_x, scale_y);


    //which gives the largest scale for which the min_space requirements can be met, but we want a 'sensible' scale, rather than 0.28457239...
    //eg if working_scale = 0.115, then we want to use 0.1, similarly 7.65 -> 5, and 76.5 -> 50

    float exponent = floor(log10(working_scale));                       //if working_scale = a * 10^b, what is b?
    working_scale *= pow(10, -exponent);                                //now find what 'a' is.

    float valid_scales[2][8] = {{1, 1.25, 2, 2.5, 3.75, 5, 7.5, 10},    //equate to 1:10, 1:8, 1:5, 1:4, 3:8, 1:2, 3:4, 1:1
                                {1, 1.5, 2, 3, 4, 5, 8, 10}};           //equate to 1:1, 3:2, 2:1, 3:1, 4:1, 5:1, 8:1, 10:1

    int i = 7;
    while (valid_scales[(exponent>=0)][i] > working_scale)              //choose closest value smaller than 'a' from list.
        i -= 1;                                                         //choosing top list if exponent -ve, bottom list for +ve exponent

    scale = valid_scales[(exponent>=0)][i] * pow(10, exponent);         //now have the appropriate scale, reapply the *10^b
}


void OrthoViews::add_view(int rel_x, int rel_y)             // add a new view to the layout
{
    bool duplicate = false;

    // check there's not already a view in this position
    for (int i = 0; i < views.size(); i++)
        if (views[i].rel_x == rel_x && views[i].rel_y == rel_y)
        {
            duplicate = true;
            break;
        }

    if (!duplicate)
    {
        string view_name = parent_doc->getUniqueObjectName("Ortho").c_str();

        char temp[15];
        sprintf(temp, "Ortho_%i_%i", rel_x, rel_y);         // label name for view, based on relative position

        bool orth_pos = ((rel_x * rel_y) == 0);             // is the rel position an orthographic one?
        orthoView * view = new orthoView(view_name, string(temp), page_name.c_str(), part_name.c_str(), & bbox);
        view_struct new_view = {orth_pos, rel_x, rel_y, view, true};
        views.push_back(new_view);

        max_r_x = max(max_r_x, rel_x);
        min_r_x = min(min_r_x, rel_x);
        max_r_y = max(max_r_y, rel_y);
        min_r_y = min(min_r_y, rel_y);

        num_gaps_x = max_r_x - min_r_x + 2;
        num_gaps_y = max_r_y - min_r_y + 2;

        int i = views.size() - 1;
        views[i].view->hidden(hidden);
        views[i].view->smooth(smooth);

        if (orth_pos)
            set_orientation(i);
        else
            set_Axo(rel_x, rel_y);

        process_views();
    }
}


void OrthoViews::del_view(int rel_x, int rel_y)             // remove a view from the layout
{
    int index = 0;
    min_r_x = max_r_x = 0;
    min_r_y = max_r_y = 0;

    for (int i = 1; i < views.size(); i++)              // start from 1 - the 0 is the primary view
    {
        if (views[i].rel_x == rel_x && views[i].rel_y == rel_y)
            index = i;                                  // index of view being removed
        else
        {
            min_r_x = min(min_r_x, views[i].rel_x);     // calculate extremes from remaining views
            max_r_x = max(max_r_x, views[i].rel_x);
            min_r_y = min(min_r_y, views[i].rel_y);
            max_r_y = max(max_r_y, views[i].rel_y);
        }
    }

    num_gaps_x = max_r_x - min_r_x + 2;
    num_gaps_y = max_r_y - min_r_y + 2;

    if (index != 0)
    {
        views[index].view->deleteme();
        delete views[index].view;
        views.erase(views.begin() + index);
        process_views();
    }
}


void OrthoViews::del_all()
{
    for (int i = views.size() - 1; i >= 0; i--)          // count downwards to delete from back
    {
        views[i].view->deleteme();
        delete views[i].view;
        views.pop_back();
    }
}


void OrthoViews::process_views()                            // update scale and positions of views
{
    if (autodims)
    {
        calc_layout_size();
        calc_scale();
        calc_offsets();
    }

    set_views();
    Command::updateActive();
    Command::commitCommand();
}


int OrthoViews::is_Ortho(int rel_x, int rel_y)
{
    int result = index(rel_x, rel_y);

    if (result != -1)
        result = views[result].ortho;

    return result;
}


int OrthoViews::index(int rel_x, int rel_y)                 // index in vector of view, -1 if doesn't exist
{
    int index = -1;

    for (int i = 0; i < views.size(); i++)
        if (views[i].rel_x == rel_x && views[i].rel_y == rel_y)
        {
            index = i;
            break;
        }

    return index;
}


void OrthoViews::set_Axo_scale(int rel_x, int rel_y, float axo_scale)
{
    int num = index(rel_x, rel_y);

    if (num != -1 && !views[num].ortho)
    {
        views[num].auto_scale = false;
        views[num].axo_scale = axo_scale;
        views[num].view->setScale(axo_scale);
        views[num].view->setPos();

        Command::updateActive();
        Command::commitCommand();
    }
}


void OrthoViews::set_Axo(int rel_x, int rel_y, gp_Dir up, gp_Dir right, bool away, int axo, bool tri)   // set custom axonometric view
{
    int     num = index(rel_x, rel_y);
    double  rotations[2];
    gp_Dir  dir;

    views[num].ortho = false;
    views[num].up = up;
    views[num].right = right;
    views[num].away = away;
    views[num].tri = tri;
    views[num].axo = axo;

    if (axo == 0)
    {
        rotations[0] = -0.7853981633974476;
        rotations[1] = -0.6154797086703873;
    }
    else if (axo == 1)
    {
        rotations[0] = -0.7853981633974476;
        rotations[1] = -0.2712637537260206;
    }
    else if (tri)
    {
        rotations[0] = -1.3088876392502007;
        rotations[1] = -0.6156624905260762;
    }
    else
    {
        rotations[0] = - M_PI_2l + 1.3088876392502007;
        rotations[1] = -0.6156624905260762;
    }

    if (away)
        rotations[1] = - rotations[1];

    gp_Ax2  cs = gp_Ax2(gp_Pnt(0,0,0), right);
    cs.SetYDirection(up);

    dir = cs.YDirection();
    cs.Rotate(gp_Ax1(gp_Pnt(0,0,0), dir), rotations[0]);

    dir = cs.XDirection();
    cs.Rotate(gp_Ax1(gp_Pnt(0,0,0), dir), rotations[1]);

    views[num].view->set_projection(cs);
    views[num].view->setPos();

    Command::updateActive();
    Command::commitCommand();
}


void OrthoViews::set_Axo(int rel_x, int rel_y)              // set view to default axo projection
{
    int num = index(rel_x, rel_y);

    if (num != -1)
    {
        gp_Dir up = primary.YDirection();                   // default to view from up and right
        gp_Dir right = primary.XDirection();
        bool away = false;

        if (rel_x * rel_y != 0)                             // but change default if it's a diagonal position
        {
            if (rotate_coeff == 1)                          // third angle
            {
                away = (rel_y < 0);

                if (rel_x < 0)
                    right = primary.Direction();
                else
                    right = primary.XDirection();
            }
            else                                            // first angle
            {
                away = (rel_y > 0);

                if (rel_x > 0)
                    right = primary.Direction();
                else
                    right = primary.XDirection();
            }
        }
        set_Axo(rel_x, rel_y, up, right, away);
    }
}


void OrthoViews::set_Ortho(int rel_x, int rel_y)            // return view to orthographic
{
    int num = index(rel_x, rel_y);

    if (num != -1 && rel_x * rel_y == 0)
    {
        views[num].ortho = true;
        set_orientation(num);
        views[num].view->setPos();

        Command::updateActive();
        Command::commitCommand();
    }
}


bool OrthoViews::get_Axo(int rel_x, int rel_y, int & axo, gp_Dir & up, gp_Dir & right, bool & away, bool & tri, float & axo_scale)
{
    int num = index(rel_x, rel_y);

    if (num != -1 && !views[num].ortho)
    {
        axo = views[num].axo;
        up = views[num].up;
        right = views[num].right;
        away = views[num].away;
        tri = views[num].tri;
        if (views[num].auto_scale)
            axo_scale = scale;
        else
            axo_scale = views[num].axo_scale;
        return true;
    }
    else
        return false;
}


void OrthoViews::auto_dims(bool setting)
{
    autodims = setting;
    if (autodims)
        process_views();
}


void OrthoViews::set_configs(float configs[5])              // for autodims off, set scale & positionings
{
    if (!autodims)
    {
        scale = configs[0];
        offset_x = configs[1];
        offset_y = configs[2];
        gap_x = configs[3];
        gap_y = configs[4];
        process_views();
    }
}


void OrthoViews::get_configs(float configs[5])              // get scale & positionings
{
    configs[0] = scale;
    configs[1] = offset_x;
    configs[2] = offset_y;
    configs[3] = gap_x;
    configs[4] = gap_y;
}




///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////


TaskOrthoViews::TaskOrthoViews(QWidget *parent)
  : ui(new Ui_TaskOrthoViews)
{
    ui->setupUi(this);
	vector<App::DocumentObject*> obj = Gui::Selection().getObjectsOfType(Part::Feature::getClassTypeId());
    const char * part = obj.front()->getNameInDocument();

    App::Document * doc = App::GetApplication().getActiveDocument();
    vector<App::DocumentObject*> pages = doc->getObjectsOfType(Drawing::FeaturePage::getClassTypeId());
    string PageName = pages.front()->getNameInDocument();
    const char * page = PageName.c_str();


    // **********************************************************************
    // note that checkboxes are numbered increasing right & down
    // while OrthoViews relative positions are increasing right & up
    // doh!  I should renumber the checkboxes for clarity
    // **********************************************************************

    //   [x+2][y+2]
    c_boxes[0][2] = ui->cb02;       //left most, x = -2, y = 0
    c_boxes[1][1] = ui->cb11;
    c_boxes[1][2] = ui->cb12;
    c_boxes[1][3] = ui->cb13;
    c_boxes[2][0] = ui->cb20;       //top most, x = 0, y = -2
    c_boxes[2][1] = ui->cb21;
    c_boxes[2][2] = ui->cb22;       //centre (primary view) checkbox x = y = 0.
    c_boxes[2][3] = ui->cb23;
    c_boxes[2][4] = ui->cb24;       //bottom most, x = 0, y = 2
    c_boxes[3][1] = ui->cb31;
    c_boxes[3][2] = ui->cb32;
    c_boxes[3][3] = ui->cb33;
    c_boxes[4][2] = ui->cb42;       //right most, x = 2, y = 0

    for (int i=0; i < 5; i++)
        for (int j=0; j < 5; j++)
            if ((abs(i-2) + abs(j-2)) < 3)                          //if i,j combination corresponds to valid check box, then proceed with:
            {
                connect(c_boxes[i][j], SIGNAL(toggled(bool)), this, SLOT(cb_toggled(bool)));
                connect(c_boxes[i][j], SIGNAL(customContextMenuRequested(const QPoint&)),this, SLOT(ShowContextMenu(const QPoint&)));
            }

    // access scale / position QLineEdits via array
    inputs[0] = ui->scale_0;
    inputs[1] = ui->x_1;
    inputs[2] = ui->y_2;
    inputs[3] = ui->spacing_h_3;
    inputs[4] = ui->spacing_v_4;

    for (int i=0; i < 5; i++)
        connect(inputs[i], SIGNAL(returnPressed()), this, SLOT(data_entered()));


    ui->tabWidget->setTabEnabled(1,false);

    connect(ui->projection, SIGNAL(currentIndexChanged(int)), this, SLOT(projectionChanged(int)));
    connect(ui->smooth, SIGNAL(stateChanged(int)), this, SLOT(smooth(int)));
    connect(ui->hidden, SIGNAL(stateChanged(int)), this, SLOT(hidden(int)));
    connect(ui->auto_tog, SIGNAL(stateChanged(int)), this, SLOT(toggle_auto(int)));

    connect(ui->view_from, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));
    connect(ui->axis_right, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));

    connect(ui->axoProj, SIGNAL(activated(int)), this, SLOT(change_axo(int)));
    connect(ui->axoUp, SIGNAL(activated(int)), this, SLOT(change_axo(int)));
    connect(ui->axoRight, SIGNAL(activated(int)), this, SLOT(change_axo(int)));
    connect(ui->vert_flip, SIGNAL(clicked()), this, SLOT(axo_button()));
    connect(ui->tri_flip, SIGNAL(clicked()), this, SLOT(axo_button()));
    connect(ui->axoScale, SIGNAL(returnPressed()), this, SLOT(axo_scale()));


    gp_Dir facing = gp_Dir(1, 0, 0);
    gp_Dir right = gp_Dir(0, 1, 0);
    orthos = new OrthoViews(page, part);
    orthos->set_primary(facing, right);
} //end of constructor


TaskOrthoViews::~TaskOrthoViews()
{
    delete orthos;
    delete ui;
}


void TaskOrthoViews::ShowContextMenu(const QPoint& pos)
{
    QString name = sender()->objectName().right(2);
    char letter = name.toStdString()[0];
    int dx = letter - '0' - 2;

    letter = name.toStdString()[1];
    int dy = letter - '0' - 2;

    if (c_boxes[dx + 2][dy + 2]->isChecked())
    {
        QPoint globalPos = c_boxes[dx + 2][dy + 2]->mapToGlobal(pos);

        QMenu myMenu;
        QString str_1 = QString::fromUtf8("Make axonometric...");
        QString str_2 = QString::fromUtf8("Edit axonometric settings...");
        QString str_3 = QString::fromUtf8("Make orthographic");


        if (orthos->is_Ortho(dx, -dy))
            myMenu.addAction(str_1);
        else
        {
            myMenu.addAction(str_2);
            if (dx * dy == 0)
                myMenu.addAction(str_3);
        }


        QAction * selectedItem = myMenu.exec(globalPos);
        if (selectedItem)
        {
            QString text = selectedItem->text();

            if (text == str_1)                          // make axo
            {
                //num_axo += 1;
                orthos->set_Axo(dx, -dy);
                axo_r_x = dx;
                axo_r_y = dy;
                ui->tabWidget->setTabEnabled(1, true);
                ui->tabWidget->setCurrentIndex(1);
                setup_axo_tab();
            }
            else if (text == str_2)                     // edit axo
            {
                axo_r_x = dx;
                axo_r_y = dy;
                ui->tabWidget->setTabEnabled(1, true);
                ui->tabWidget->setCurrentIndex(1);
                setup_axo_tab();
            }
            else if (text == str_3)                     // make ortho
            {
                orthos->set_Ortho(dx, -dy);
                //num_axo -= 1;

                if (dx == axo_r_x && dy == axo_r_y)
                {
                    axo_r_x = 0;
                    axo_r_y = 0;
                    ui->tabWidget->setTabEnabled(1, false);
                }
            }
        }
    }
}


void TaskOrthoViews::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
}


void TaskOrthoViews::cb_toggled(bool toggle)
{
    QString name = sender()->objectName().right(2);
    char letter = name.toStdString()[0];
    int dx = letter - '0' - 2;

    letter = name.toStdString()[1];
    int dy = letter - '0' - 2;

    if (toggle)
    {
        orthos->add_view(dx, -dy);
        if (dx * dy != 0)                      // adding an axo view
        {
            //num_axo += 1;
            axo_r_x = dx;
            axo_r_y = dy;
            ui->tabWidget->setTabEnabled(1, true);
            ui->tabWidget->setCurrentIndex(1);
            setup_axo_tab();
        }
    }
    else                                        // removing a view
    {
        if (!orthos->is_Ortho(dx, -dy))         // is it an axo one?
        {
            //num_axo -= 1;
            if (dx == axo_r_x && dy == axo_r_y) // is it the one currently being edited?
            {
                axo_r_x = 0;
                axo_r_y = 0;
                ui->tabWidget->setTabEnabled(1, false);
            }
        }
        orthos->del_view(dx, -dy);
    }

    set_configs();
}


void TaskOrthoViews::projectionChanged(int index)
{
    int proj = 3 - 2 * index;       // index = 0 = third angle
    orthos->set_projection(proj);

    set_configs();
}


void TaskOrthoViews::setPrimary(int dir)
{
    int p_sel = ui->view_from->currentIndex();        // index for entry selected for 'view from'
    int r_sel = ui->axis_right->currentIndex();         // index for entry selected for 'rightwards axis'

    int p_vec[3] = {0, 0, 0};                       // will be the vector for 'view from'
    int r_vec[3] = {0, 0, 0};                       // will be vector for 'rightwards axis'
    int r[2] = {0, 1};

    int pos = 1 - 2 * int(p_sel / 3);               // 1 if p_sel = 0, 1, 2  or -1 if p_sel = 3, 4, 5
    p_sel = p_sel % 3;                              // p_sel = 0, 1, 2
    p_vec[p_sel] = pos;

    for (int i = p_sel; i < 2; i++)                 // make r[2] to be, {0, 1}, {0, 2}, or {1, 2} depending upon p_sel
        r[i] += 1;

    pos = 1 - 2 * int(r_sel / 2);                   // 1 if r_sel = 0, 1  or -1 if r_sel = 3, 4
    r_sel = r_sel % 2;                              // r_sel = 0, 1
    r_vec[r[r_sel]] = pos;

    gp_Dir facing = gp_Dir(p_vec[0], p_vec[1], p_vec[2]);
    gp_Dir right = gp_Dir(r_vec[0], r_vec[1], r_vec[2]);

    orthos->set_primary(facing, right);


    // update rightwards combobox in case of 'view from' change
    if (sender() == ui->view_from)
    {
        disconnect(ui->axis_right, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));

        QStringList items;
        items << QString::fromUtf8("X +ve") << QString::fromUtf8("Y +ve") << QString::fromUtf8("Z +ve");
        items << QString::fromUtf8("X -ve") << QString::fromUtf8("Y -ve") << QString::fromUtf8("Z -ve");
        items.removeAt(p_sel + 3);
        items.removeAt(p_sel);

        ui->axis_right->clear();
        ui->axis_right->addItems(items);
        ui->axis_right->setCurrentIndex(r_sel - pos + 1);

        connect(ui->axis_right, SIGNAL(currentIndexChanged(int)), this, SLOT(setPrimary(int)));
    }

    set_configs();
}


void TaskOrthoViews::hidden(int i)
{
    orthos->set_hidden(i == 2);
}


void TaskOrthoViews::smooth(int i)
{
    orthos->set_smooth(i == 2);
}


void TaskOrthoViews::toggle_auto(int i)
{
    if (i == 2)                                 //auto scale switched on
    {
        orthos->auto_dims(true);
        ui->label_4->setEnabled(false);
        ui->label_5->setEnabled(false);
        ui->label_6->setEnabled(false);

        for (int j = 0; j < 5; j++)
            inputs[j]->setEnabled(false);       //disable user input boxes
    }
    else
    {
        orthos->auto_dims(false);
        ui->label_4->setEnabled(true);
        ui->label_5->setEnabled(true);
        ui->label_6->setEnabled(true);

        for (int j = 0; j < 5; j++)
            inputs[j]->setEnabled(true);        //enable user input boxes
    }
}


void TaskOrthoViews::data_entered()
{
    bool ok;

    QString name = sender()->objectName().right(1);
    char letter = name.toStdString()[0];
    int index = letter - '0';

    float value = inputs[index]->text().toFloat(&ok);
    if (ok)
    {
        if (data[index] != value)
        {
            data[index] = value;
            orthos->set_configs(data);
        }
    }
    else
    {
        inputs[index]->setText(QString::number(data[index]));
        return;
    }
}


bool TaskOrthoViews::user_input()
{
    //if user presses return, this is intercepted by FreeCAD which interprets it as activating the 'OK' button
    //if return was pressed in a text box though, we don't want it to do 'OK', so check to see if a text box has been modified.
    bool modified = false;

    for (int i = 0; i < 5; i++)
    {
        modified = inputs[i]->isModified();         //has input box been modified?
        if (modified)
        {
            inputs[i]->setModified(false);          //reset modified flag
            break;                                  //stop checking
        }
    }
    if (ui->axoScale->isModified())
    {
        ui->axoScale->setModified(false);
        modified = true;
    }
    return modified;
}


void TaskOrthoViews::clean_up(bool keep)
{
    if (!keep)           //user cancelled the drawing
        orthos->del_all();
}


void TaskOrthoViews::setup_axo_tab()
{
    int     axo;
    gp_Dir  up, right;
    bool    away, tri;
    float   axo_scale;
    int     up_n, right_n;

    orthos->get_Axo(axo_r_x, -axo_r_y, axo, up, right, away, tri, axo_scale);

    // convert gp_Dirs into selections of comboboxes
    if (up.X() != 0)
        up_n = (up.X() == -1) ? 3 : 0;
    else if (up.Y() != 0)
        up_n = (up.Y() == -1) ? 4 : 1;
    else
        up_n = (up.Z() == -1) ? 5 : 2;

    if (right.X() != 0)
        right_n = (right.X() == -1) ? 3 : 0;
    else if (right.Y() != 0)
        right_n = (right.Y() == -1) ? 4 : 1;
    else
        right_n = (right.Z() == -1) ? 5 : 2;

    if (right_n > (up_n % 3 + 3))
        right_n -= 2;
    else if (right_n > up_n)
        right_n -= 1;

    QStringList items;
    items << QString::fromUtf8("X +ve") << QString::fromUtf8("Y +ve") << QString::fromUtf8("Z +ve");
    items << QString::fromUtf8("X -ve") << QString::fromUtf8("Y -ve") << QString::fromUtf8("Z -ve");
    items.removeAt(up_n % 3 + 3);
    items.removeAt(up_n % 3);

    ui->axoUp->setCurrentIndex(up_n);
    ui->axoRight->clear();
    ui->axoRight->addItems(items);
    ui->axoRight->setCurrentIndex(right_n);

    ui->vert_flip->setChecked(away);
    ui->tri_flip->setChecked(tri);
    ui->axoProj->setCurrentIndex(axo);
    ui->axoScale->setText(QString::number(axo_scale));
}


void TaskOrthoViews::change_axo(int p)
{
    int u_sel = ui->axoUp->currentIndex();        // index for entry selected for 'view from'
    int r_sel = ui->axoRight->currentIndex();         // index for entry selected for 'rightwards axis'

    int u_vec[3] = {0, 0, 0};               // will be the vector for 'view from'
    int r_vec[3] = {0, 0, 0};               // will be vector for 'rightwards axis'
    int r[2] = {0, 1};

    int pos = 1 - 2 * int(u_sel / 3);       // 1 if p_sel = 0,1,2  or -1 if p_sel = 3,4,5
    u_sel = u_sel % 3;                      // p_sel = 0,1,2
    u_vec[u_sel] = pos;

    for (int i = u_sel; i < 2; i++)
        r[i] += 1;

    pos = 1 - 2 * int(r_sel / 2);
    r_sel = r_sel % 2;
    r_vec[r[r_sel]] = pos;

    gp_Dir up = gp_Dir(u_vec[0], u_vec[1], u_vec[2]);
    gp_Dir right = gp_Dir(r_vec[0], r_vec[1], r_vec[2]);

    orthos->set_Axo(axo_r_x, -axo_r_y, up, right, ui->vert_flip->isChecked(), ui->axoProj->currentIndex(), ui->tri_flip->isChecked());

    if (ui->axoProj->currentIndex() == 2)
        ui->tri_flip->setEnabled(true);
    else
        ui->tri_flip->setEnabled(false);


    QStringList items;
    items << QString::fromUtf8("X +ve") << QString::fromUtf8("Y +ve") << QString::fromUtf8("Z +ve");
    items << QString::fromUtf8("X -ve") << QString::fromUtf8("Y -ve") << QString::fromUtf8("Z -ve");
    items.removeAt(u_sel % 3 + 3);
    items.removeAt(u_sel % 3);

    ui->axoRight->clear();
    ui->axoRight->addItems(items);
    ui->axoRight->setCurrentIndex(r_sel - pos + 1);
}


void TaskOrthoViews::axo_button()
{
    change_axo();
}


void TaskOrthoViews::axo_scale()
{
    bool ok;
    QString temp = ui->axoScale->text();

    float value = temp.toFloat(&ok);
    if (ok)
        orthos->set_Axo_scale(axo_r_x, -axo_r_y, value);
}


void TaskOrthoViews::set_configs()
{
    orthos->get_configs(data);

    for (int i = 0; i < 5; i++)
        inputs[i]->setText(QString::number(data[i]));
}




//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgOrthoViews::TaskDlgOrthoViews()
    : TaskDialog()
{
    widget = new TaskOrthoViews();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("actions/drawing-orthoviews"), widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskDlgOrthoViews::~TaskDlgOrthoViews()
{
}

//==== calls from the TaskView ===============================================================


void TaskDlgOrthoViews::open()
{
}

void TaskDlgOrthoViews::clicked(int)
{
}

bool TaskDlgOrthoViews::accept()
{
    bool check = widget->user_input();
    if (!check)
        widget->clean_up(true);

    return !check;
}

bool TaskDlgOrthoViews::reject()
{
    widget->clean_up(false);
    return true;
}




#include "moc_TaskOrthoViews.cpp"
