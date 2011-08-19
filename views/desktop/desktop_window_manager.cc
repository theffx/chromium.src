// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "views/desktop/desktop_window_manager.h"

#include "views/events/event.h"
#include "views/widget/widget.h"
#include "ui/gfx/point.h"
#include "ui/gfx/rect.h"
#include "views/widget/widget.h"
#include "views/widget/native_widget_private.h"
#include "views/widget/native_widget_views.h"
#include "views/widget/widget_delegate.h"
#if defined(OS_LINUX)
#include "views/window/hit_test.h"
#endif
#include "views/window/non_client_view.h"

namespace {

class MoveWindowController : public views::desktop::WindowController {
 public:
  MoveWindowController(views::Widget* widget, const gfx::Point& start)
      : target_(widget),
        offset_(start) {
  }

  virtual ~MoveWindowController() {
  }

  bool OnMouseEvent(const views::MouseEvent& event) {
    if (event.type()== ui::ET_MOUSE_DRAGGED) {
      gfx::Point origin = event.location().Subtract(offset_);
      gfx::Rect rect = target_->GetWindowScreenBounds();
      rect.set_origin(origin);
      target_->SetBounds(rect);
      return true;
    }
    return false;
  }

 private:
  views::Widget* target_;
  gfx::Point offset_;

  DISALLOW_COPY_AND_ASSIGN(MoveWindowController);
};

// Simple resize controller that handle all resize as if the bottom
// right corner is selected.
class ResizeWindowController : public views::desktop::WindowController {
 public:
  ResizeWindowController(views::Widget* widget)
      : target_(widget) {
  }

  virtual ~ResizeWindowController() {
  }

  bool OnMouseEvent(const views::MouseEvent& event) OVERRIDE {
    if (event.type()== ui::ET_MOUSE_DRAGGED) {
      gfx::Point location = event.location();
      gfx::Rect rect = target_->GetWindowScreenBounds();
      gfx::Point size = location.Subtract(rect.origin());
      target_->SetSize(gfx::Size(std::max(10, size.x()),
                                 std::max(10, size.y())));
      return true;
    }
    return false;
  }

 private:
  views::Widget* target_;

  DISALLOW_COPY_AND_ASSIGN(ResizeWindowController);
};

}  // namespace

namespace views {
namespace desktop {

WindowController::WindowController() {
}

WindowController::~WindowController() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowManager, public:

DesktopWindowManager::DesktopWindowManager(Widget* desktop)
    : desktop_(desktop),
      mouse_capture_(NULL) {
}

DesktopWindowManager::~DesktopWindowManager() {
}

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowManager, WindowManager implementation:

void DesktopWindowManager::StartMoveDrag(
    views::Widget* widget,
    const gfx::Point& point) {
  DCHECK(!window_controller_.get());
  DCHECK(!HasMouseCapture());
  if (!widget->IsMaximized() && !widget->IsMinimized()) {
    gfx::Point new_point = point;
    if (desktop_->non_client_view()) {
      gfx::Rect client =
          desktop_->non_client_view()->frame_view()->GetBoundsForClientView();
      new_point.Offset(client.x(), client.y());
    }
    SetMouseCapture();
    window_controller_.reset(new MoveWindowController(widget, new_point));
  }
}

void DesktopWindowManager::StartResizeDrag(
    views::Widget* widget, const gfx::Point& point, int hittest_code) {
  DCHECK(!window_controller_.get());
  DCHECK(!HasMouseCapture());
  if (!widget->IsMaximized() &&
      !widget->IsMinimized() &&
      (widget->widget_delegate() || widget->widget_delegate()->CanResize())) {
    SetMouseCapture();
    window_controller_.reset(new ResizeWindowController(widget));
  }
}

bool DesktopWindowManager::SetMouseCapture(views::Widget* widget) {
  if (mouse_capture_)
    return false;
  if (mouse_capture_ == widget)
    return true;
  DCHECK(!HasMouseCapture());
  SetMouseCapture();
  mouse_capture_ = widget;
  return true;
}

bool DesktopWindowManager::ReleaseMouseCapture(views::Widget* widget) {
  if (!widget || mouse_capture_ != widget)
    return false;
  DCHECK(HasMouseCapture());
  ReleaseMouseCapture();
  mouse_capture_ = NULL;
  return true;
}

bool DesktopWindowManager::HasMouseCapture(const views::Widget* widget) const {
  return widget && mouse_capture_ == widget;
}

bool DesktopWindowManager::HandleMouseEvent(
    views::Widget* widget, const views::MouseEvent& event) {

  if (window_controller_.get()) {
    if (!window_controller_->OnMouseEvent(event)) {
      ReleaseMouseCapture();
      window_controller_.reset();
    }
    return true;
  }

  if (mouse_capture_) {
    views::MouseEvent translated(event, widget->GetRootView(),
                                 mouse_capture_->GetRootView());
    mouse_capture_->OnMouseEvent(translated);
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
// DesktopWindowManager, private:

void DesktopWindowManager::SetMouseCapture() {
  return desktop_->native_widget_private()->SetMouseCapture();
}

void DesktopWindowManager::ReleaseMouseCapture() {
  return desktop_->native_widget_private()->ReleaseMouseCapture();
}

bool DesktopWindowManager::HasMouseCapture() const {
  return desktop_->native_widget_private()->HasMouseCapture();
}

}  // namespace desktop
}  // namespace views
