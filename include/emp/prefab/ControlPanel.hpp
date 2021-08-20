#ifndef EMP_CONTROL_PANEL_HPP
#define EMP_CONTROL_PANEL_HPP

#include "emp/base/optional.hpp"
#include "emp/base/vector.hpp"

#include "emp/prefab/ButtonGroup.hpp"
#include "emp/prefab/FontAwesomeIcon.hpp"
#include "emp/prefab/ToggleButtonGroup.hpp"

#include "emp/tools/string_utils.hpp"

#include "emp/web/Animate.hpp"
#include "emp/web/Element.hpp"
#include "emp/web/Div.hpp"

namespace emp::prefab {

  namespace internal {
    using checker_func_t = std::function<bool(const web::Animate &)>;

    class ControlPanelInfo : public web::internal::DivInfo {

      std::string refresh_unit;
      std::map<std::string, int> refresh_rates{
        {"MILLISECONDS", 100}, {"FRAMES", 5}
      };

      const std::map<std::string, const checker_func_t> refresh_checkers{
        { "MILLISECONDS",
          [elapsed_milliseconds = 0, this]
          (const web::Animate & anim) mutable {
            int rate = this->refresh_rates[this->refresh_unit];
            elapsed_milliseconds += anim.GetStepTime();
            if (elapsed_milliseconds > rate) {
              elapsed_milliseconds -= rate;
              if (elapsed_milliseconds > rate) elapsed_milliseconds = 0;
              return true;
            }
            return false;
          }},
        { "FRAMES",
          [this](const web::Animate & anim) {
            return anim.GetFrameCount() % this->refresh_rates[this->refresh_unit];
          }
        }
      };

      checker_func_t do_redraw;

      emp::vector<web::Widget> refresh_list;
      std::function<void()> simulation;

      public:
      ControlPanelInfo(const std::string & in_id="")
      : DivInfo(in_id),
      refresh_unit("MILLISECONDS"),
      do_redraw(refresh_checkers.at(refresh_unit)),
      simulation([](){ ; })  { ; }

      const checker_func_t & GetRedrawChecker() const {
        return do_redraw;
      }

      void SetSimulation(const std::function<void()> & sim) {
        simulation = sim;
      }

      const std::function<void()> & GetSimulation() const {
        return simulation;
      }

      void SetUnit(const std::string & unit) {
        refresh_unit = unit;
        do_redraw = refresh_checkers.at(refresh_unit);
      }

      void SetRate(const int & rate) {
        refresh_rates[refresh_unit] = rate;
      }

      emp::vector<web::Widget> & GetRefreshList() {
        return refresh_list;
      }
    };
  }

  class ControlPanel : public web::Div {

    /**
     * Get shared info pointer, cast to ControlPanel-specific type.
     * @return cast pointer
     */
    internal::ControlPanelInfo * Info() {
      return dynamic_cast<internal::ControlPanelInfo *>(info);
    }

    /**
     * Get shared info pointer, cast to const ControlPanel-specific type.
     * @return cast pointer
     */
    const internal::ControlPanelInfo * Info() const {
      return dynamic_cast<internal::ControlPanelInfo *>(info);
    }

    ToggleButtonGroup toggle_run;
    Div button_line;
    web::Button step;

    protected:
    ControlPanel(
      const std::string & refresh_mode,
      const int & refresh_rate,
      web::internal::DivInfo * in_info
    ) : web::Div(in_info),
    toggle_run{
      FontAwesomeIcon{"fa-play"}, FontAwesomeIcon{"fa-pause"},
      "success", "warning",
      true, false,
      emp::to_string(GetID(), "_run_toggle")
    },
    button_line(ButtonGroup{emp::to_string(GetID(), "_core")}),
    step{
      [](){ ; },
      "<span class=\"fa fa-step-forward\" aria-hidden=\"true\"></span>",
      emp::to_string(GetID(), "_step")
    }
    {
      AddAttr(
        "class", "btn-toolbar",
        "class", "space_groups",
        "role", "toolbar",
        "aria-label", "Toolbar with simulation controls"
      );
      SetRefreshUnit(refresh_mode);
      SetRefreshRate(refresh_rate);

      static_cast<Div>(*this) << button_line;
      button_line << toggle_run;
      button_line << step;

      step.AddAttr(
        "class", "btn",
        "class", "btn-success",
        "disabled", true
      );

      AddAnimation(GetID(),
        [&run_sim=GetSimulation(),
          &refresh_list=Info()->GetRefreshList(),
          &do_redraw=Info()->GetRedrawChecker()]
        (const web::Animate & anim) mutable {
          // Run the simulation function every frame
          run_sim();
          // Redraw widgets according to a rule
          if(do_redraw(anim)) {
            std::cout << "List size " << refresh_list.size() << std::endl;
            for (auto & wid : refresh_list) {
              std::cout << "Redrawing " << wid.GetID() << std::endl;
              wid.Redraw();
            }
          }
        }
      );

      toggle_run.SetCallback(
        [&anim=Animate(GetID()), step=web::Button(step)]
        (bool is_active) mutable {
          if (is_active) {

            anim.Start();
          } else {
            anim.Stop();
          }
        }
      );

      step.SetCallback([&anim=Animate(GetID())]() {
        anim.Step();
      });
    }

    public:
    ControlPanel(
      const std::string & refresh_mode,
      const int & refresh_rate,
      const std::string & in_id="")
    : ControlPanel(
      refresh_mode,
      refresh_rate,
      new internal::ControlPanelInfo(in_id)
    ) { ; }

    ControlPanel & SetSimulation(const std::function<void()> & sim) {
      Info()->SetSimulation(sim);
      return *this;
    }

    const std::function<void()> & GetSimulation() const {
      return Info()->GetSimulation();
    }

    ControlPanel & SetRefreshUnit(const std::string & units) {
      Info()->SetUnit(units);
      return *this;
    }

    void SetRefreshRate(const int & val) {
      Info()->SetRate(val);
    }

    void AddToRefreshList(Widget & area) {
      Info()->GetRefreshList().push_back(area);
      std::cout << Info()->GetRefreshList().size() << std::endl;
    }

    template <typename IN_TYPE>
    ControlPanel & operator<<(IN_TYPE && in_val) {
      // Took soooo long to figure out but if in_val is a r-value ref
      // IN_TYPE is just the TYPE. If it's l-value then it's TYPE &.
      // std::decay and forward help handle both.
      if constexpr(std::is_same<typename std::decay<IN_TYPE>::type, web::Button>::value ||
        std::is_same<typename std::decay<IN_TYPE>::type, ToggleButtonGroup>::value) {
        button_line << std::forward<IN_TYPE>(in_val);
      } else if constexpr(std::is_same<typename std::decay<IN_TYPE>::type, ButtonGroup>::value) {
        button_line = std::forward<ButtonGroup>(in_val);
        static_cast<Div>(*this) << button_line;
      } else {
        static_cast<Div>(*this) << std::forward<IN_TYPE>(in_val);
      }
      return (*this);
    }
  };
}

#endif