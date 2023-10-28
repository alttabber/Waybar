#include "modules/privacy/privacy_item.hpp"

#include <fmt/core.h>
#include <pipewire/pipewire.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>

#include "AModule.hpp"
#include "glibmm/main.h"
#include "glibmm/priorities.h"
#include "gtkmm/enums.h"
#include "gtkmm/label.h"
#include "gtkmm/revealer.h"
#include "gtkmm/tooltip.h"
#include "sigc++/adaptors/bind.h"
#include "util/gtk_icon.hpp"
#include "util/pipewire/privacy_node_info.hpp"

namespace waybar::modules::privacy {

PrivacyItem::PrivacyItem(const Json::Value& config_,
                         enum util::PipewireBackend::PrivacyNodeType privacy_type_,
                         const std::string& pos)
    : Gtk::Revealer(),
      privacy_type(privacy_type_),
      mutex_(),
      box_(Gtk::ORIENTATION_HORIZONTAL, 0),
      icon_() {
  switch (privacy_type) {
    case util::PipewireBackend::PRIVACY_NODE_TYPE_AUDIO_INPUT:
      box_.get_style_context()->add_class("audio-in");
      iconName = "waybar-privacy-audio-input-symbolic";
      break;
    case util::PipewireBackend::PRIVACY_NODE_TYPE_AUDIO_OUTPUT:
      box_.get_style_context()->add_class("audio-out");
      iconName = "waybar-privacy-audio-output-symbolic";
      break;
    case util::PipewireBackend::PRIVACY_NODE_TYPE_VIDEO_INPUT:
      box_.get_style_context()->add_class("screenshare");
      iconName = "waybar-privacy-screen-share-symbolic";
      break;
    default:
    case util::PipewireBackend::PRIVACY_NODE_TYPE_NONE:
      enabled = false;
      return;
  }

  // Set the reveal transition to not look weird when sliding in
  if (pos == "modules-left") {
    set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_RIGHT);
  } else if (pos == "modules-center") {
    set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_CROSSFADE);
  } else if (pos == "modules-right") {
    set_transition_type(Gtk::REVEALER_TRANSITION_TYPE_SLIDE_LEFT);
  }

  box_.set_name("privacy-item");
  box_.add(icon_);
  add(box_);

  // Icon Name
  if (config_["enabled"].isBool()) {
    enabled = config_["enabled"].asBool();
  }

  // Icon Name
  if (config_["icon-name"].isString()) {
    iconName = config_["icon-name"].asString();
  }
  icon_.set_from_icon_name(iconName, Gtk::ICON_SIZE_INVALID);

  property_child_revealed().signal_changed().connect(
      sigc::mem_fun(*this, &PrivacyItem::on_child_revealed_changed));
  signal_map().connect(sigc::mem_fun(*this, &PrivacyItem::on_map_changed));

  // Don't show by default
  set_reveal_child(true);
  set_visible(false);
}

bool PrivacyItem::is_enabled() { return enabled; }

void PrivacyItem::on_child_revealed_changed() {
  if (!this->get_child_revealed()) {
    set_visible(false);
  }
}

void PrivacyItem::on_map_changed() {
  if (this->get_visible()) {
    set_reveal_child(true);
  }
}

void PrivacyItem::set_in_use(bool in_use) {
  mutex_.lock();
  if (this->in_use == in_use && init) {
    mutex_.unlock();
    return;
  }

  if (init) {
    this->in_use = in_use;
    if (this->in_use) {
      set_visible(true);
      // The `on_map_changed` callback will call `set_reveal_child(true)`
      // when the widget is realized so we don't need to call that here.
      // This fixes a bug where the revealer wouldn't start the animation
      // due to us changing the visibility at the same time.
    } else {
      set_reveal_child(false);
      // The `on_child_revealed_changed` callback will call `set_visible(false)`
      // when the animation has finished so we don't need to call that here.
      // We do this so that the widget gets hidden after the revealer hide animation
      // has finished.
    }
  } else {
    set_visible(false);
    set_reveal_child(false);
  }
  this->init = true;

  // CSS status class
  const std::string status = this->in_use ? "in-use" : "";
  // Remove last status if it exists
  if (!lastStatus.empty() && get_style_context()->has_class(lastStatus)) {
    get_style_context()->remove_class(lastStatus);
  }
  // Add the new status class to the Box
  if (!status.empty() && !get_style_context()->has_class(status)) {
    get_style_context()->add_class(status);
  }
  lastStatus = status;

  mutex_.unlock();
}

void PrivacyItem::set_icon_size(uint size) { icon_.set_pixel_size(size); }

}  // namespace waybar::modules::privacy
