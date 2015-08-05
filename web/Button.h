#ifndef EMP_WEB_BUTTON_H
#define EMP_WEB_BUTTON_H

//////////////////////////////////////////////////////////////////////////////////////////
//
//  Specs for the Button widget.
//

#include "Widget.h"

namespace emp {
namespace web {

  class Button;

  class ButtonInfo : public internal::WidgetInfo {
    friend Button;
  protected:
    std::string label;
    std::string title;
      
    bool autofocus;
    bool disabled;
    
    uint32_t callback_id;
    std::string onclick_info;
    
    ButtonInfo(const std::string & in_id="") : internal::WidgetInfo(in_id) { ; }
    ButtonInfo(const ButtonInfo &) = delete;               // No copies of INFO allowed
    ButtonInfo & operator=(const ButtonInfo &) = delete;   // No copies of INFO allowed
    virtual ~ButtonInfo() {
      if (callback_id) emp::JSDelete(callback_id);  // Delete callback wrapper.
    }

    virtual void GetHTML(std::stringstream & HTML) override {
      HTML.str("");                                           // Clear the current text.
      HTML << "<button";                                      // Start the button tag.
      if (title != "") HTML << " title=\"" << title << "\"";  // Add a title if there is one.
      if (disabled) { HTML << " disabled=true"; }             // Check if should be disabled
      HTML << " id=\"" << id << "\"";                         // Indicate ID.
      HTML << " onclick=\"" << onclick_info << "\"";          // Indicate action on click.
      HTML << ">" << label << "</button>";                    // Close and label the button.
    }
      
    void UpdateCallback(const std::function<void()> & in_cb) {
      if (callback_id) emp::JSDelete(callback_id);    // Delete previous callback wrapper.
      callback_id = JSWrap(in_cb);                    // Save id for callback trigger.
      onclick_info = std::string("emp.Callback(") + std::to_string(callback_id) + ")";
      if (active) ReplaceHTML();                      // If node is active, immediately redraw!
    }
    void UpdateCallback(const std::string in_cb_info) {
      if (callback_id) emp::JSDelete(callback_id);    // Delete previous callback wrapper.
      callback_id = 0;                                // No ID currently in callback.
      onclick_info = in_cb_info;
      if (active) ReplaceHTML();                      // If node is active, immediately redraw!
    }

    void UpdateLabel(const std::string & in_label) {
      label = in_label;
      if (active) ReplaceHTML();                      // If node is active, immediately redraw!
    }

    void UpdateTitle(const std::string & in_title) {
      title = in_title;
      if (active) ReplaceHTML();                      // If node is active, immediately redraw!
    }

    void UpdateAutofocus(bool in_af) {
      autofocus = in_af;
      if (active) ReplaceHTML();                      // If node is active, immediately redraw!
    }

    void UpdateDisabled(bool in_dis) {
      disabled = in_dis;
      if (active) ReplaceHTML();                      // If node is active, immediately redraw!
    }


  public:
    virtual std::string GetType() override { return "web::ButtonInfo"; }
  };


  class Button : public internal::WidgetFacet<Button> {
    friend ButtonInfo;
  protected:
    // Get a properly cast version of indo.  
    ButtonInfo * Info() { return (ButtonInfo *) info; }
    const ButtonInfo * Info() const { return (ButtonInfo *) info; }
 
    Button(ButtonInfo * in_info) : WidgetFacet(in_info) { ; }

  public:
    // If we provide all info about the button, create the ButtonInfo object now.
    Button(const std::string & in_cb_info, const std::string & in_label,
           const std::string & in_id="")
      : WidgetFacet(in_id)
    {
      Info()->label = in_label;
      Info()->title = "";
      Info()->autofocus = false;
      Info()->disabled = false;
      Info()->callback_id = 0;
      Info()->onclick_info = in_cb_info;
    }
    Button(const std::function<void()> & in_cb, const std::string & in_label,
           const std::string & in_id="")
      : Button(std::string(""), in_label, in_id)
    {
      Info()->callback_id = JSWrap(in_cb);
      Info()->onclick_info = emp::to_string("emp.Callback(", Info()->callback_id, ")");
    }
    Button(const Button & in) : WidgetFacet(in) { ; }
    virtual ~Button() { ; }

    Button & Callback(const std::function<void()> & in_cb) {
      Info()->UpdateCallback(in_cb);
      return *this;
    }
    Button & Callback(const std::string in_cb_info) {
      Info()->UpdateCallback(in_cb_info);
      return *this;
    }
    Button & Label(const std::string & in_label) { Info()->UpdateLabel(in_label); return *this; }
    Button & Title(const std::string & in_t) { Info()->UpdateTitle(in_t); return *this; }
    Button & Autofocus(bool in_af) { Info()->UpdateAutofocus(in_af); return *this; }
    Button & Disabled(bool in_dis) { Info()->UpdateDisabled(in_dis); return *this; }
    
    const std::string & GetLabel() const { return Info()->label; }
    const std::string & GetTitle() const { return Info()->title; }
    bool HasAutofocus() const { return Info()->autofocus; }
    bool IsDisabled() const { return Info()->disabled; }
  };


};
};

#endif
