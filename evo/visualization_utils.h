#ifndef VISUALIZATION_UTILS_H
#define VISUALIZATION_UTILS_H

#include "../web/web_init.h"
#include "../web/Animate.h"
#include "../emtools/JSWrap.h"
#include "../config/config.h"
#include "../tools/BitSet.h"
#include "../tools/Random.h"
#include "../tools/stats.h"
#include "../tools/string_utils.h"
#include "../tools/FunctionSet.h"
#include "../tools/const.h"

#include "../web/d3/selection.h"
#include "../web/d3/scales.h"
#include "../web/d3/axis.h"
#include "../web/d3/svg_shapes.h"
#include "../web/d3/layout.h"

#include <functional>
#include <algorithm>
#include <deque>

#include "NK-const.h"

//Pretty sure D3VisualizationInfo can't be shared among multiple D3Visualizations

namespace emp{
namespace web{
class D3Visualization : public internal::WidgetFacet<D3Visualization> {
    friend class D3VisualizationInfo;
protected:
  class D3VisualizationInfo : public internal::WidgetInfo {
    friend D3Visualization;

    protected:
      int width;
      int height;
      D3Visualization * parent;
      D3::Selection * svg;

      D3VisualizationInfo(D3Visualization * parent, const std::string & in_id="") : internal::WidgetInfo(in_id) {
        this->parent = parent;
      }

      D3VisualizationInfo(const D3VisualizationInfo &) = delete;
      D3VisualizationInfo & operator=(const D3VisualizationInfo &) = delete;
      virtual ~D3VisualizationInfo(){;}

      virtual bool IsD3VisualiationInfo() const override {return true;}

      virtual void GetHTML(std::stringstream & HTML) override {
        HTML.str("");
        HTML << "<svg id=\"" << id
             << "\" width=\"" << width
             << "\" height=\"" << height << "\">";

        HTML << "</svg>";
      }

      // Trigger any JS code needed on re-draw.
      void TriggerJS() override {
        if (state == Widget::ACTIVE) {            // Only draw on active canvases
          svg = new D3::Selection(D3::Select("#"+id));
          parent->Setup();
        }
      }

    public:
      virtual std::string GetType() override {return "web::D3VisualizationInfo";}

  };

  // Get a properly cast version of indo.
  D3VisualizationInfo * Info() { return (D3VisualizationInfo *) info; }
  const D3VisualizationInfo * Info() const { return (D3VisualizationInfo *) info; }

  D3Visualization(D3VisualizationInfo * in_info) : WidgetFacet(in_info) { ; }

public:

  D3Visualization(int w, int h, const std::string & in_id="")
        : WidgetFacet(in_id) {
      #ifdef EMSCRIPTEN
      Initialize();
      n_objects();
      #endif

      info = new D3VisualizationInfo(this, in_id);
      Info()->width = w;
      Info()->height = h;
  }

  D3Visualization(const D3Visualization & in) : WidgetFacet(in) { ; }
  D3Visualization(const Widget & in) : WidgetFacet(in) { emp_assert(info->IsD3VisualiationInfo()); }
  virtual ~D3Visualization() { ; }

  using INFO_TYPE = D3VisualizationInfo;

  int GetWidth() const { return Info()->width; }
  int GetHeight() const { return Info()->height; }
  D3::Selection * GetSVG() {return Info()->svg;}
  std::string GetID() {return Info()->id;}

  int POP_SIZE = 100;
  int MAX_GENS = 1000;
  emp::vector<std::string> variables;
  virtual void Setup(){;}

  virtual void AnimateStep(...){;}
  virtual void AnimateStep(emp::vector<double>){;}
  virtual void AnimateStep(int parent, int offspring){;}
};

class FitnessVisualization : public D3Visualization {
private:
  double margin = 10;
  double axis_width = 40;
  double fitness_growth_margin = 1.5;
  double fitness_loss_margin = .8;
public:
  D3::LinearScale * x_scale;
  D3::LinearScale * fitness_scale;
  D3::Axis<D3::LinearScale> * ax;
  D3::Selection * circles;
  D3::ToolTip * tip;

  std::function<double(double, int, int)> scaled_d = [&](double d, int i, int k){
      return fitness_scale->ApplyScale(d);
  };

  std::function<double(double, int, int)> scaled_i = [&](double d, int i, int k){
      return x_scale->ApplyScale(i);
  };

  FitnessVisualization(int w=500, int h=500) : D3Visualization(w, h){;}

  void Setup(){

    D3::Selection * svg = GetSVG();

    double lowest = 10;//*(std::min_element(fitnesses.begin(), fitnesses.end()));
    double highest = 20;//*(std::max_element(fitnesses.begin(), fitnesses.end()));

    //Set up scales
    fitness_scale = new D3::LinearScale();
    x_scale = new D3::LinearScale();
    fitness_scale->SetDomain(std::array<double, 2>({highest*fitness_growth_margin, lowest*fitness_loss_margin}));
    fitness_scale->SetRange(std::array<double, 2>({margin, GetHeight() - margin}));
    x_scale->SetDomain(std::array<double, 2>({0, (double)POP_SIZE-1}));
    x_scale->SetRange(std::array<double, 2>({axis_width, GetHeight()-margin}));

    //Set up axis
    ax = new D3::Axis<D3::LinearScale>();
    ax->SetScale(*fitness_scale);
    ax->SetOrientation("right");
    ax->Draw(*svg);

    //Make callback functions
    JSWrap(scaled_d, GetID()+"scaled_d");
    JSWrap(scaled_i, GetID()+"scaled_i");
  }

  void AnimateStep(emp::vector<double> fitnesses){
      //Draw circles that represent fitnesses
      circles = new D3::Selection(GetSVG()->SelectAll("circle").Data(fitnesses));
      circles->EnterAppend("circle");
      circles->ExitRemove();
      circles->SetAttr("r", 5);
      circles->SetAttr("cx", GetID()+"scaled_i");
      circles->SetAttr("cy", GetID()+"scaled_d");
      circles->SetStyle("fill", "green");

     // circles->AddToolTip(tip);

      circles = new D3::Selection(circles->Data(fitnesses));
      circles->Transition().SetAttr("cy", GetID()+"scaled_d");
  }

};

class GraphVisualization : public D3Visualization {
private:
  double y_margin = 10;
  double x_margin = 30;
  double axis_width = 60;
  double y_min = 1000;
  double y_max = 0;
  double x_min = 0;
  double x_max = 0;

public:
  D3::LinearScale * x_scale;
  D3::LinearScale * y_scale;
  D3::Axis<D3::LinearScale> * x_axis;
  D3::Axis<D3::LinearScale> * y_axis;

  //Callback function for taking a datapoint and getting appropriately scaled y val
  std::function<double(std::array<double, 2>, int, int)> y = [this](std::array<double, 2> d, int i=0, int k=0){
      return y_scale->ApplyScale(d[1]);
  };

  //Callback function for taking a datapoint and getting appropriately scaled x val
  std::function<double(std::array<double,2>, int, int)> x = [this](std::array<double, 2> d, int i=0, int k=0){
      return x_scale->ApplyScale(d[0]);
  };

  //Callback function for getting unscaled x value of data point (used as key function for data binding)
  std::function<double(std::array<double,2>, int)> return_x = [&](std::array<double, 2> d, int i=0){
      return d[0];
  };

  //Callback function for drawing data after rescale animation
  std::function<void(int, int)> draw_data = [this](int i=0, int j=0, int k=0){
    DrawData(true);
  };

  //Format function for tooltip
  D3::FormatFunction rounded = D3::FormatFunction(".2f");

  //Function telling tooltip how to display a data point
  std::function<std::string(std::array<double, 2>, int, int)> tooltip_display =
                                       [this](std::array<double, 2> d, int i, int k) {
                                         return to_string(rounded(d[1]));
                                     };

  GraphVisualization(std::string y_var, std::string x_var, int w=800, int h=400) : D3Visualization(w, h){
    this->variables.push_back(x_var);
    this->variables.push_back(y_var);
  }

  GraphVisualization(std::string y_var, int w=800, int h=400) : D3Visualization(w, h){
    this->variables.push_back("Update");
    this->variables.push_back(y_var);
  }

  std::deque<std::array<std::array<double, 2>, 1> > data;
  std::array<std::array<double, 2>, 1> prev_data  = std::array<std::array<double, 2>, 1>({std::array<double, 2>({-1,-1})});
  D3::LineGenerator * make_line;

  D3::ToolTip* tip;
  D3::Selection t;

  virtual void Setup(){
    EM_ASM({emp["waiting"] = 0});
    D3::Selection * svg = GetSVG();

    //Wrap ncessary callback functions
    JSWrap(tooltip_display, GetID()+"tooltip_display");
    JSWrap(x, GetID()+"x");
    JSWrap(y, GetID()+"y");
    JSWrap(return_x, GetID()+"return_x");
    JSWrap(draw_data, GetID()+"draw_data");


    //Create tool top
    tip = new D3::ToolTip(GetID()+"tooltip_display");
    GetSVG()->SetupToolTip(*tip);
    //Set up scales
    y_scale = new D3::LinearScale();
    x_scale = new D3::LinearScale();
    y_scale->SetDomain(std::array<double, 2>({y_max, y_min}));
    y_scale->SetRange(std::array<double, 2>({y_margin, (double)GetHeight() - axis_width}));
    x_scale->SetDomain(std::array<double, 2>({0, (double)MAX_GENS}));
    x_scale->SetRange(std::array<double, 2>({axis_width, GetWidth()-x_margin}));

    //Set up axes
    x_axis = new D3::Axis<D3::LinearScale>(variables[0]);
    x_axis->SetScale(*x_scale);
    y_axis = new D3::Axis<D3::LinearScale>(variables[1]);
    y_axis->SetScale(*y_scale);
    D3::DrawAxes(*x_axis, *y_axis, *svg);
    make_line = new D3::LineGenerator();
  }

  virtual void AnimateStep(emp::vector<double> data_point) {
    data.push_back(std::array<std::array<double, 2>, 1>({data_point[0], data_point[1]}));
    D3::Selection * svg = GetSVG();

    if (data_point[1] > y_max || data_point[1] < y_min
        || data_point[0] > x_max || data_point[0] < x_min) {
      y_max = std::max(data_point[1]*1.2, y_max);
      y_min = std::min(data_point[1]*.8, y_min);
      x_max = std::max(data_point[0]*1.2, x_max);
      x_min = std::min(data_point[0]*.8, x_min);

      if (y_min == y_max) {
        y_min -= .2;
        y_max += .2;
      }

      if (x_min == x_max) {
        x_min -= .2;
        x_max += .2;
      }


      t = svg->Transition();
      y_axis->Rescale(y_max, y_min, t);
      x_axis->Rescale(x_min, x_max, t);

      Redraw(t);

      t.Each("end", GetID()+"draw_data");
    } else {
      DrawData(false);
    }
  }

  void Redraw(D3::Selection & s) {
    s.SelectAll(".data-point").SetAttr("cy", GetID()+"y");
    s.SelectAll(".data-point").SetAttr("cx", GetID()+"x");

    EM_ASM_ARGS({
      circle_data = js.objects[$0].selectAll(".data-point").data();
      path_data = [];
      for (iter=0; iter<circle_data.length-1; iter++){
        path_data.push(js.objects[$1]([[emp[Pointer_stringify($3)+"x"](circle_data[iter],0,0), emp[Pointer_stringify($3)+"y"](circle_data[iter],0,0)],
                     [emp[Pointer_stringify($3)+"x"](circle_data[iter+1],0,0), emp[Pointer_stringify($3)+"y"](circle_data[iter+1],0,0)]]));
      }
      js.objects[$0].selectAll(".line-seg").data(path_data);
      js.objects[$2].selectAll(".line-seg").attr("d", function(d){return d;});
    }, GetSVG()->GetID(), make_line->GetID(), s.GetID(), this->GetID().c_str());
  }

  void DrawData(bool backlog = false) {

    //If there's a backlog, then we're only allowed to clear it if this
    //was called recursively or from jacascript (since javascript handles)
    //using this as a callback to asynchronous stuff)
    if ((!backlog && data.size() > 1) || data.size() == 0){
      return;
    }

    //We can't draw a line on the first update
    if (prev_data[0][0] >=0 ){
      std::array<std::array<double, 2>, 2> line_data;
      prev_data[0][0] = x(prev_data[0], 0, 0);
      prev_data[0][1] = y(prev_data[0],0 ,0);
      line_data[0] = prev_data[0];

      std::array<std::array<double, 2>, 1> new_data;
      new_data = data[0];
      new_data[0][0] = x(new_data[0],0,0);
      new_data[0][1] = y(new_data[0],0,0);
      line_data[1] = new_data[0];

      D3::Selection line = make_line->DrawShape(line_data, *GetSVG());
      line.SetAttr("fill", "none");
      line.SetAttr("stroke", "green");
      line.SetAttr("stroke-width", 1);
      line.SetAttr("class", "line-seg");
    }

    D3::Selection enter = GetSVG()->SelectAll(".data-point").Data(data[0], GetID()+"return_x").EnterAppend("circle");
    enter.SetAttr("cy", GetID()+"y");
    enter.SetAttr("cx", GetID()+"x");
    enter.SetAttr("r", 2);
    enter.SetAttr("class", "data-point");
    enter.SetStyle("fill", "green");
    enter.BindToolTipMouseover(*tip);
    prev_data = data[0];
    data.pop_front();
    if (data.size() > 0) {
      DrawData(true);
    }
  }

  void Clear() {
    data.clear();
    GetSVG()->SelectAll(".data-point").Data({}).ExitRemove();
    GetSVG()->SelectAll(".line-seg").Data({}).ExitRemove();
    y_axis->Rescale(0, 1000, *(GetSVG()));
    x_axis->Rescale(0, 0, *(GetSVG()));
    y_min = 1000;
    y_max = 0;
    x_min = 0;
    x_max = 0;
    prev_data  = {{-1,-1}};
  }
};

class LineageVisualization : public D3Visualization {
protected:
  double y_margin = 10;
  double x_margin = 30;

public:

  struct LineageTreeNode {
    EMP_BUILD_INTROSPECTIVE_TUPLE( double, x,
                                   double, y,
                                   int, name,
				                   int, parent,
                                   int, depth,
                                   std::string, genome,
                                   int, loc,
                                   bool, alive,
                                   bool, persist
                                )
  };


  struct LineageTreeEdge {
    EMP_BUILD_INTROSPECTIVE_TUPLE( LineageTreeNode, source,
                                     LineageTreeNode, target)
  };

  D3::TreeLayout<LineageTreeNode> tree;
  D3::JSObject alive;
  D3::ToolTip * tip;
  D3::JSONDataset * data;
  FunctionSet<void> pending_funcs;

  bool init = false;
  int next_pos;
  int next_parent = 0;
  int next_child;
  std::string next_genome;

  std::function<std::string(LineageTreeNode, int, int)> color_fun = [](LineageTreeNode d, int i = 0, int k = 0){
    if (d.alive()){
      return "red";
    } else if (d.persist()) {
      return "blue";
    } else {
      return "black";
    }
  };

  std::function<std::string(LineageTreeEdge, int, int)> color_fun_link = [this](LineageTreeEdge d, int i = 0, int k = 0){
    return "black";
  };

  std::function<std::string(LineageTreeNode, int, int)> tooltip_display = [](LineageTreeNode d, int i = 0, int k = 0) {
    return "Name: " + to_string(d.name()) + "<br>Genome: " + to_string(d.genome());
  };

  LineageVisualization(int width, int height) : D3Visualization(width, height){variables.push_back("Persist");}

  virtual void Setup() {
    JSWrap(color_fun, GetID()+"color_fun");
    JSWrap(tooltip_display, GetID()+"tooltip_display");

    data = new D3::JSONDataset();
    tip = new D3::ToolTip(GetID()+"tooltip_display");
    GetSVG()->Move(0,0);
    data->Append(std::string("{\"name\": 0, \"parent\": \"null\", \"alive\":false, \"loc\": -1, \"persist\":false, \"genome\":\"none\", \"children\" : []}"));
    tree.SetDataset(data);
    tree.SetSize(GetHeight(), GetWidth());

    EM_ASM_ARGS({
      js.objects[$0] = [js.objects[$1][0]];
    }, alive.GetID(), data->GetID());

    init = true;
    pending_funcs.Run();
  }

  void LoadDataFromFile(std::string filename) {
    if (init) {
      data->LoadDataFromFile(filename, [this](){DrawTree();});
    } else {
      pending_funcs.Add([this, filename](){
          data->LoadDataFromFile(filename, [this](){
              DrawTree();
          });
      });
    }
  }

  virtual void AnimateStep(emp::vector<double> persist) {

    for (double val : persist) {
      (void) val;
      EM_ASM_ARGS({
        js.objects[$1](js.objects[$0][0], $2).persist = true;
      }, data->GetID(), data->FindInHierarchy.GetID(), val);

    }
  }

  virtual void AnimateStep(int parent, int child){
    std::string child_json = std::string("{\"name\":" + to_string(child) + ", \"parent\":" + to_string(parent) + ", \"alive\":true, \"loc\":" + to_string(next_pos) + ", \"persist\":false, \"genome\":\"" + next_genome + "\", \"children\":[]}");
    int pos = data->AppendNestedFromList(child_json, alive);
    (void) pos;

    EM_ASM_ARGS({
        while (js.objects[$0].length < $1 + 1) {
          js.objects[$0].push(-1);
        }
        js.objects[$0][$1].alive = false;
        js.objects[$0][$1] = js.objects[$0][$2].children[js.objects[$0][$2].children.length-1];
    }, alive.GetID(), next_pos, pos);
    DrawTree();
  }

  virtual void DrawTree() {
    D3::Selection nodeEnter = tree.GenerateNodesAndLinks(*GetSVG());
    nodeEnter.Append("circle").SetAttr("r", 2).AddToolTip(*tip);
    GetSVG()->SelectAll("g.node").SelectAll("circle").SetStyle("fill", GetID()+"color_fun");
    GetSVG()->SelectAll(".link").SetStyle("stroke", GetID()+"color_fun_link");
  }

  void RecordPlacement(int pos) {
    next_pos = pos + 1; //alive array has extra element for null parent
    AnimateStep(next_parent, next_child);
  }

  template <typename ORG>
  void RecordParent(int parent, int child, ORG* org) {
    next_parent = parent;
    next_child = child;
    next_genome = to_string(*org);
  }
};

class SpatialGridLineageVisualization : public LineageVisualization {

public:

  const int grid_width = 10;
  const int grid_height = 10;
  const int legend_cell_size = 15;

  D3::Selection legend;

  struct LegendNode {
    EMP_BUILD_INTROSPECTIVE_TUPLE( int, loc)
  };

  std::function<std::string(LineageTreeNode, int, int)> color_fun = [this](LineageTreeNode d, int i = 0, int k = 0){
    if (d.loc() < 0) {
      return std::string("black");
    }

    double x = (d.loc() % grid_width) - grid_width/2;
    double y = (d.loc() / grid_width) - grid_height/2;

    double r = sqrt(emp::pow(x,2)+emp::pow(y,2)) / sqrt(emp::pow(grid_width,2)+emp::pow(grid_height,2));
    (void) r;

    //atan2 takes sign into account
    double theta = atan2(y, x)*(180/emp::PI);
    (void) theta;

    char * color = (char *) EM_ASM_INT({
        var text = d3.hcl($1, 150, $0*175).toString();
	    var buffer = Module._malloc(text.length+1);
	    Module.writeStringToMemory(text, buffer);
	    return buffer;
    }, r, theta);

    std::string result = std::string(color);
    free(color);
    return result;
  };

  std::function<std::string(LineageTreeNode, int, int)> dark_color_fun = [this](LineageTreeNode d, int i = 0, int k = 0){
    if (d.loc() < 0) {
      return std::string("black");
    }

    double x = (d.loc() % grid_width) - grid_width/2;
    double y = (d.loc() / grid_width) - grid_height/2;

    double r = sqrt(emp::pow(x,2)+emp::pow(y,2)) / sqrt(emp::pow(grid_width,2)+emp::pow(grid_height,2));
    (void) r;

    //atan2 takes sign into account
    double theta = atan2(y, x)*(180/emp::PI);
    (void) theta;

    char * color = (char *) EM_ASM_INT({
        var text = d3.hcl($1, 150, $0*175).darker().toString();
      var buffer = Module._malloc(text.length+1);
      Module.writeStringToMemory(text, buffer);
      return buffer;
    }, r, theta);

    std::string result = std::string(color);
    free(color);
    return result;
  };

  std::function<std::string(LineageTreeEdge, int, int)> color_fun_link = [this](LineageTreeEdge d, int i = 0, int k = 0){
    return this->color_fun(d.source(),0,0);
  };

  std::function<std::string(LineageTreeNode, int, int)> tooltip_display = [this](LineageTreeNode d, int i = 0, int k = 0) {
    return "ID: " + to_string(d.name()) + ", Pos: (" + to_string(d.loc()% grid_width) + ", " + to_string(d.loc()/grid_width) + ")" + "<br>Genome: " + to_string(d.genome());
  };

  std::function<int(LegendNode, int, int)> get_x = [this](LegendNode d, int i = 0, int k = 0) {
    return legend_cell_size*(d.loc() % grid_width);
  };

  std::function<int(LegendNode, int, int)> get_y = [this](LegendNode d, int i = 0, int k = 0) {
    return legend_cell_size*(d.loc() / grid_width);
  };

  std::function<void(int)> legend_mouseover = [this](D3::Selection d) {
    EM_ASM_ARGS({emp.filter_fun = function(d){return d.loc != js.objects[$0].data()[0].loc;}}, d.GetID());
    legend.SelectAll("rect").Filter("filter_fun").SetClassed("faded", true);
    GetSVG()->SelectAll(".node").Filter("filter_fun").SetClassed("faded", true);
    EM_ASM_ARGS({emp.filter_fun = function(d){return d.source.loc != js.objects[$0].data()[0].loc;}}, d.GetID());
    GetSVG()->SelectAll(".link").Filter("filter_fun").SetClassed("faded", true);
  };

  std::function<void(int)> legend_mouseout = [this](D3::Selection d) {
    EM_ASM_ARGS({emp.filter_fun = function(d){return d.loc != js.objects[$0].data()[0].loc;}}, d.GetID());
    legend.SelectAll("rect").Filter("filter_fun").SetClassed("faded", false);
    GetSVG()->SelectAll(".node").Filter("filter_fun").SetClassed("faded", false);
    EM_ASM_ARGS({emp.filter_fun = function(d){return d.source.loc != js.objects[$0].data()[0].loc;}}, d.GetID());
    GetSVG()->SelectAll(".link").Filter("filter_fun").SetClassed("faded", false);
  };

  SpatialGridLineageVisualization(int width, int height) : LineageVisualization(width, height){;}

  virtual void Setup() {
    LineageVisualization::Setup();
    JSWrap(color_fun, GetID()+"color_fun");
    JSWrap(dark_color_fun, GetID()+"dark_color_fun");
    JSWrap(color_fun_link, GetID()+"color_fun_link");
    JSWrap(tooltip_display, GetID()+"tooltip_display");
    JSWrap(legend_mouseover, GetID()+"legend_mouseover");
    JSWrap(legend_mouseout, GetID()+"legend_mouseout");
    JSWrap(get_x, GetID()+"get_x");
    JSWrap(get_y, GetID()+"get_y");
    delete tip;
    tip = new D3::ToolTip(GetID()+"tooltip_display");
    legend = D3::Select("body").Append("svg");

    legend.SetAttr("x", 1000).SetAttr("y", 0).SetAttr("width", legend_cell_size*grid_width).SetAttr("height", legend_cell_size*grid_height);
    legend.SetStyle("position", "fixed").SetStyle("right", "10px").SetStyle("top", "10px");//.SetStyle("width", "10%");//.SetStyle("height", "10%");
    emp::vector<LegendNode> legend_data(grid_width*grid_height);
    for (int i = 0; i < grid_width*grid_height; ++i) {
      legend_data[i].loc(i);
    }

    legend.SelectAll("rect").Data(legend_data)
                            .EnterAppend("rect")
                            .SetStyle("fill", GetID()+"color_fun")
                            .SetStyle("stroke", GetID()+"color_fun")
                            .SetStyle("stroke-width", 1)
                            .SetAttr("width", legend_cell_size)
                            .SetAttr("height", legend_cell_size)
                            .SetAttr("x", GetID()+"get_x")
                            .SetAttr("y", GetID()+"get_y")
                            .On("mouseover", GetID()+"legend_mouseover")
                            .On("mouseout", GetID()+"legend_mouseout");
  }

};

}
}
#endif
