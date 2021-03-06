// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_UI_CHOOSE_MOBILE_NETWORK_DIALOG_H_
#define CHROME_BROWSER_CHROMEOS_UI_CHOOSE_MOBILE_NETWORK_DIALOG_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/web_dialogs/web_dialog_delegate.h"

namespace chromeos {

// Dialog for manual selection of cellular network.
class ChooseMobileNetworkDialog : public ui::WebDialogDelegate {
 public:
  // Shows the dialog box.
  static void ShowDialog(gfx::NativeWindow owning_window);

 private:
  ChooseMobileNetworkDialog();

  // Overridden from ui::WebDialogDelegate:
  virtual ui::ModalType GetDialogModalType() const override;
  virtual base::string16 GetDialogTitle() const override;
  virtual GURL GetDialogContentURL() const override;
  virtual void GetWebUIMessageHandlers(
      std::vector<content::WebUIMessageHandler*>* handlers) const override;
  virtual void GetDialogSize(gfx::Size* size) const override;
  virtual std::string GetDialogArgs() const override;
  virtual void OnDialogClosed(const std::string& json_retval) override;
  virtual void OnCloseContents(
      content::WebContents* source, bool* out_close_dialog) override;
  virtual bool ShouldShowDialogTitle() const override;
  virtual bool HandleContextMenu(
      const content::ContextMenuParams& params) override;

  DISALLOW_COPY_AND_ASSIGN(ChooseMobileNetworkDialog);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_UI_CHOOSE_MOBILE_NETWORK_DIALOG_H_
