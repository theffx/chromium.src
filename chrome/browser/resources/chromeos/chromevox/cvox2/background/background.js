// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview The entry point for all ChromeVox2 related code for the
 * background page.
 */

goog.provide('Background');
goog.provide('global');

goog.require('AutomationPredicate');
goog.require('AutomationUtil');
goog.require('Output');
goog.require('Output.EventType');
goog.require('cursors.Cursor');
goog.require('cvox.ChromeVoxEditableTextBase');
goog.require('cvox.TabsApiHandler');

// Define types here due to editable_text.js's implicit dependency with
// ChromeVoxEventWatcher.
/** @type {Object} */
cvox.ChromeVoxEventWatcher;
/** @type {function(boolean)} */
cvox.ChromeVoxEventWatcher.handleTextChanged;
/** @type {function()} */
cvox.ChromeVoxEventWatcher.setUpTextHandler;

goog.scope(function() {
var AutomationNode = chrome.automation.AutomationNode;
var Dir = AutomationUtil.Dir;
var EventType = chrome.automation.EventType;

/** Classic Chrome accessibility API. */
global.accessibility =
    chrome.accessibilityPrivate || chrome.experimental.accessibility;

/**
 * ChromeVox2 background page.
 * @constructor
 */
Background = function() {
  /**
   * A list of site substring patterns to use with ChromeVox next. Keep these
   * strings relatively specific.
   * @type {!Array.<string>}
   * @private
   */
  this.whitelist_ = ['chromevox_next_test'];

  /**
   * @type {cvox.TabsApiHandler}
   * @private
   */
  this.tabsHandler_ = new cvox.TabsApiHandler(cvox.ChromeVox.tts,
                                              cvox.ChromeVox.braille,
                                              cvox.ChromeVox.earcons);

  /**
   * @type {cursors.Range}
   * @private
   */
  this.currentRange_ = null;

  /**
   * Whether ChromeVox Next is active.
   * @type {boolean}
   * @private
   */
  this.active_ = false;

  // Only needed with unmerged ChromeVox classic loaded before.
  global.accessibility.setAccessibilityEnabled(false);

  // Manually bind all functions to |this|.
  for (var func in this) {
    if (typeof(this[func]) == 'function')
      this[func] = this[func].bind(this);
  }

  /**
   * Maps an automation event to its listener.
   * @type {!Object.<EventType, function(Object) : void>}
   */
  this.listeners_ = {
    alert: this.onEventDefault,
    focus: this.onEventDefault,
    hover: this.onEventDefault,
    menuStart: this.onEventDefault,
    menuEnd: this.onEventDefault,
    loadComplete: this.onLoadComplete,
    textChanged: this.onTextOrTextSelectionChanged,
    textSelectionChanged: this.onTextOrTextSelectionChanged,
    valueChanged: this.onEventDefault
  };

  // Register listeners for ...
  // Desktop.
  chrome.automation.getDesktop(this.onGotTree);

  // Tabs.
  chrome.tabs.onUpdated.addListener(this.onTabUpdated);
};

Background.prototype = {
  /**
   * Handles chrome.tabs.onUpdated.
   * @param {number} tabId
   * @param {Object} changeInfo
   */
  onTabUpdated: function(tabId, changeInfo) {
    if (changeInfo.status != 'complete')
      return;
    chrome.tabs.get(tabId, function(tab) {
      if (!tab.url)
        return;

      var next = this.isWhitelisted_(tab.url);

      this.toggleChromeVoxVersion({next: next, classic: !next});
    }.bind(this));
  },

  /**
   * Handles all setup once a new automation tree appears.
   * @param {chrome.automation.AutomationNode} root
   */
  onGotTree: function(root) {
    // Register all automation event listeners.
    for (var eventType in this.listeners_)
      root.addEventListener(eventType, this.listeners_[eventType], true);

    if (root.attributes.docLoaded) {
      this.onLoadComplete(
          {target: root, type: chrome.automation.EventType.loadComplete});
    }
  },

  /**
   * Handles chrome.commands.onCommand.
   * @param {string} command
   */
  onGotCommand: function(command) {
    if (command == 'toggleChromeVoxVersion') {
      this.toggleChromeVoxVersion();
      return;
    }

    if (!this.active_ || !this.currentRange_)
      return;

    var current = this.currentRange_;
    var dir = Dir.FORWARD;
    var pred = null;
    switch (command) {
      case 'nextHeading':
        dir = Dir.FORWARD;
        pred = AutomationPredicate.heading;
        break;
      case 'previousHeading':
        dir = Dir.BACKWARD;
        pred = AutomationPredicate.heading;
        break;
      case 'nextCharacter':
        current = current.move(cursors.Unit.CHARACTER, Dir.FORWARD);
        break;
      case 'previousCharacter':
        current = current.move(cursors.Unit.CHARACTER, Dir.BACKWARD);
        break;
      case 'nextWord':
        current = current.move(cursors.Unit.WORD, Dir.FORWARD);
        break;
      case 'previousWord':
        current = current.move(cursors.Unit.WORD, Dir.BACKWARD);
        break;
      case 'nextLine':
        current = current.move(cursors.Unit.LINE, Dir.FORWARD);
        break;
      case 'previousLine':
        current = current.move(cursors.Unit.LINE, Dir.BACKWARD);
        break;
      case 'nextLink':
        dir = Dir.FORWARD;
        pred = AutomationPredicate.link;
        break;
      case 'previousLink':
        dir = Dir.BACKWARD;
        pred = AutomationPredicate.link;
        break;
      case 'nextElement':
        current = current.move(cursors.Unit.NODE, Dir.FORWARD);
        break;
      case 'previousElement':
        current = current.move(cursors.Unit.NODE, Dir.BACKWARD);
        break;
      case 'goToBeginning':
      var node =
          AutomationUtil.findNodePost(current.getStart().getNode().root,
                                      Dir.FORWARD,
                                      AutomationPredicate.leaf);
        if (node)
          current = cursors.Range.fromNode(node);
        break;
      case 'goToEnd':
        var node =
            AutomationUtil.findNodePost(current.getStart().getNode().root,
                                        Dir.BACKWARD,
                                        AutomationPredicate.leaf);
        if (node)
          current = cursors.Range.fromNode(node);
        break;
      case 'doDefault':
        if (this.currentRange_)
          this.currentRange_.getStart().getNode().doDefault();
        break;
    }

    if (pred) {
      var node = AutomationUtil.findNextNode(
          current.getBound(dir).getNode(), dir, pred);

      if (node)
        current = cursors.Range.fromNode(node);
    }

    if (current) {
      // TODO(dtseng): Figure out what it means to focus a range.
      current.getStart().getNode().focus();

      var prevRange = this.currentRange_;
      this.currentRange_ = current;
      new Output(this.currentRange_, prevRange, Output.EventType.NAVIGATE);
    }
  },

  /**
   * Provides all feedback once ChromeVox's focus changes.
   * @param {Object} evt
   */
  onEventDefault: function(evt) {
    var node = evt.target;

    if (!node)
      return;

    var prevRange = this.currentRange_;
    this.currentRange_ = cursors.Range.fromNode(node);

    // Don't process nodes inside of web content if ChromeVox Next is inactive.
    if (node.root.role != chrome.automation.RoleType.desktop && !this.active_)
      return;

    new Output(this.currentRange_, prevRange, evt.type);
  },

  /**
   * Provides all feedback once a load complete event fires.
   * @param {Object} evt
   */
  onLoadComplete: function(evt) {
    // Don't process nodes inside of web content if ChromeVox Next is inactive.
    if (evt.target.root.role != chrome.automation.RoleType.desktop &&
        !this.active_)
      return;

    var node = AutomationUtil.findNodePost(evt.target,
        Dir.FORWARD,
        AutomationPredicate.leaf);
    if (node)
      this.currentRange_ = cursors.Range.fromNode(node);

    if (this.currentRange_)
      new Output(this.currentRange_, null, evt.type);
  },

  /**
   * Provides all feedback once a text selection change event fires.
   * @param {Object} evt
   */
  onTextOrTextSelectionChanged: function(evt) {
    // Don't process nodes inside of web content if ChromeVox Next is inactive.
    if (evt.target.root.role != chrome.automation.RoleType.desktop &&
        !this.active_)
      return;

    if (!evt.target.state.focused)
      return;

    if (!this.currentRange_) {
      this.onEventDefault(evt);
      this.currentRange_ = cursors.Range.fromNode(evt.target);
    }

    var textChangeEvent = new cvox.TextChangeEvent(
        evt.target.attributes.value,
        evt.target.attributes.textSelStart,
        evt.target.attributes.textSelEnd,
        true);  // triggered by user
    if (!this.editableTextHandler ||
        evt.target != this.currentRange_.getStart().getNode()) {
      this.editableTextHandler =
          new cvox.ChromeVoxEditableTextBase(
              textChangeEvent.value,
              textChangeEvent.start,
              textChangeEvent.end,
              evt.target.state['protected'],
              cvox.ChromeVox.tts);
    }

    this.editableTextHandler.changed(textChangeEvent);
    new Output(this.currentRange_, null, evt.type, {braille: true});
  },

  /**
   * @private
   * @param {string} url
   * @return {boolean} Whether the given |url| is whitelisted.
   */
  isWhitelisted_: function(url) {
    return this.whitelist_.some(function(item) {
      return url.indexOf(item) != -1;
    }.bind(this));
  },

  /**
   * Disables classic ChromeVox.
   * @param {number} tabId The tab where ChromeVox classic is running in.
   */
  disableClassicChromeVox_: function(tabId) {
    chrome.tabs.executeScript(
          tabId,
          {'code': 'try { window.disableChromeVox(); } catch(e) { }\n',
           'allFrames': true});
  },

  /**
   * Toggles between ChromeVox Next and Classic.
   * @param {{classic: boolean, next: boolean}=} opt_options Forceably set.
  */
  toggleChromeVoxVersion: function(opt_options) {
    if (!opt_options) {
      opt_options = {};
      opt_options.next = !this.active_;
      opt_options.classic = !opt_options.next;
    }

    if (opt_options.next) {
      if (!chrome.commands.onCommand.hasListener(this.onGotCommand))
        chrome.commands.onCommand.addListener(this.onGotCommand);

      if (!this.active_)
        chrome.automation.getTree(this.onGotTree);
      this.active_ = true;
    } else {
      if (chrome.commands.onCommand.hasListener(this.onGotCommand))
        chrome.commands.onCommand.removeListener(this.onGotCommand);

      if (this.active_) {
        for (var eventType in this.listeners_) {
          this.currentRange_.getStart().getNode().root.removeEventListener(
              eventType, this.listeners_[eventType], true);
        }
      }
      this.active_ = false;
    }

    chrome.tabs.query({active: true}, function(tabs) {
      if (opt_options.classic) {
        // This case should do nothing because Classic gets injected by the
        // extension system via our manifest. Once ChromeVox Next is enabled
        // for tabs, re-enable.
        // cvox.ChromeVox.injectChromeVoxIntoTabs(tabs);
      } else {
        tabs.forEach(function(tab) {
          this.disableClassicChromeVox_(tab.id);
        }.bind(this));
      }
    }.bind(this));
  }
};

/** @type {Background} */
global.backgroundObj = new Background();

});  // goog.scope
