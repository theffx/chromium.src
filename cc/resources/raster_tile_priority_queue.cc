// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/resources/raster_tile_priority_queue.h"

namespace cc {

namespace {

class RasterOrderComparator {
 public:
  explicit RasterOrderComparator(TreePriority tree_priority)
      : tree_priority_(tree_priority) {}

  bool operator()(
      const RasterTilePriorityQueue::PairedTilingSetQueue* a,
      const RasterTilePriorityQueue::PairedTilingSetQueue* b) const {
    // Note that in this function, we have to return true if and only if
    // a is strictly lower priority than b. Note that for the sake of
    // completeness, empty queue is considered to have lowest priority.
    if (a->IsEmpty() || b->IsEmpty())
      return b->IsEmpty() < a->IsEmpty();

    WhichTree a_tree = a->NextTileIteratorTree(tree_priority_);
    const auto* a_queue =
        a_tree == ACTIVE_TREE ? a->active_queue.get() : a->pending_queue.get();

    WhichTree b_tree = b->NextTileIteratorTree(tree_priority_);
    const auto* b_queue =
        b_tree == ACTIVE_TREE ? b->active_queue.get() : b->pending_queue.get();

    const Tile* a_tile = a_queue->Top();
    const Tile* b_tile = b_queue->Top();

    const TilePriority& a_priority =
        a_tile->priority_for_tree_priority(tree_priority_);
    const TilePriority& b_priority =
        b_tile->priority_for_tree_priority(tree_priority_);
    bool prioritize_low_res = tree_priority_ == SMOOTHNESS_TAKES_PRIORITY;

    // In smoothness mode, we should return pending NOW tiles before active
    // EVENTUALLY tiles. So if both priorities here are eventually, we need to
    // check the pending priority.
    if (prioritize_low_res &&
        a_priority.priority_bin == TilePriority::EVENTUALLY &&
        b_priority.priority_bin == TilePriority::EVENTUALLY) {
      bool a_is_pending_now =
          a_tile->priority(PENDING_TREE).priority_bin == TilePriority::NOW;
      bool b_is_pending_now =
          b_tile->priority(PENDING_TREE).priority_bin == TilePriority::NOW;
      if (a_is_pending_now || b_is_pending_now)
        return a_is_pending_now < b_is_pending_now;

      // In case neither one is pending now, fall through.
    }

    // If the bin is the same but the resolution is not, then the order will be
    // determined by whether we prioritize low res or not.
    // TODO(vmpstr): Remove this when TilePriority is no longer a member of Tile
    // class but instead produced by the iterators.
    if (b_priority.priority_bin == a_priority.priority_bin &&
        b_priority.resolution != a_priority.resolution) {
      // Non ideal resolution should be sorted lower than other resolutions.
      if (a_priority.resolution == NON_IDEAL_RESOLUTION)
        return true;

      if (b_priority.resolution == NON_IDEAL_RESOLUTION)
        return false;

      if (prioritize_low_res)
        return b_priority.resolution == LOW_RESOLUTION;
      return b_priority.resolution == HIGH_RESOLUTION;
    }

    return b_priority.IsHigherPriorityThan(a_priority);
  }

 private:
  TreePriority tree_priority_;
};

WhichTree HigherPriorityTree(TreePriority tree_priority,
                             const TilingSetRasterQueue* active_queue,
                             const TilingSetRasterQueue* pending_queue,
                             const Tile* shared_tile) {
  switch (tree_priority) {
    case SMOOTHNESS_TAKES_PRIORITY: {
      const Tile* active_tile = shared_tile ? shared_tile : active_queue->Top();
      const Tile* pending_tile =
          shared_tile ? shared_tile : pending_queue->Top();

      const TilePriority& active_priority = active_tile->priority(ACTIVE_TREE);
      const TilePriority& pending_priority =
          pending_tile->priority(PENDING_TREE);

      // If we're down to eventually bin tiles on the active tree, process the
      // pending tree to allow tiles required for activation to be initialized
      // when memory policy only allows prepaint.
      if (active_priority.priority_bin == TilePriority::EVENTUALLY &&
          pending_priority.priority_bin == TilePriority::NOW) {
        return PENDING_TREE;
      }
      return ACTIVE_TREE;
    }
    case NEW_CONTENT_TAKES_PRIORITY:
      return PENDING_TREE;
    case SAME_PRIORITY_FOR_BOTH_TREES: {
      const Tile* active_tile = shared_tile ? shared_tile : active_queue->Top();
      const Tile* pending_tile =
          shared_tile ? shared_tile : pending_queue->Top();

      const TilePriority& active_priority = active_tile->priority(ACTIVE_TREE);
      const TilePriority& pending_priority =
          pending_tile->priority(PENDING_TREE);

      if (active_priority.IsHigherPriorityThan(pending_priority))
        return ACTIVE_TREE;
      return PENDING_TREE;
    }
    default:
      NOTREACHED();
      return ACTIVE_TREE;
  }
}

}  // namespace

RasterTilePriorityQueue::RasterTilePriorityQueue() {
}

RasterTilePriorityQueue::~RasterTilePriorityQueue() {
}

void RasterTilePriorityQueue::Build(
    const std::vector<PictureLayerImpl::Pair>& paired_layers,
    TreePriority tree_priority) {
  tree_priority_ = tree_priority;
  for (std::vector<PictureLayerImpl::Pair>::const_iterator it =
           paired_layers.begin();
       it != paired_layers.end();
       ++it) {
    paired_queues_.push_back(
        make_scoped_ptr(new PairedTilingSetQueue(*it, tree_priority_)));
  }
  paired_queues_.make_heap(RasterOrderComparator(tree_priority_));
}

void RasterTilePriorityQueue::Reset() {
  paired_queues_.clear();
}

bool RasterTilePriorityQueue::IsEmpty() const {
  return paired_queues_.empty() || paired_queues_.front()->IsEmpty();
}

Tile* RasterTilePriorityQueue::Top() {
  DCHECK(!IsEmpty());
  return paired_queues_.front()->Top(tree_priority_);
}

void RasterTilePriorityQueue::Pop() {
  DCHECK(!IsEmpty());

  paired_queues_.pop_heap(RasterOrderComparator(tree_priority_));
  PairedTilingSetQueue* paired_queue = paired_queues_.back();
  paired_queue->Pop(tree_priority_);
  paired_queues_.push_heap(RasterOrderComparator(tree_priority_));
}

RasterTilePriorityQueue::PairedTilingSetQueue::PairedTilingSetQueue() {
}

RasterTilePriorityQueue::PairedTilingSetQueue::PairedTilingSetQueue(
    const PictureLayerImpl::Pair& layer_pair,
    TreePriority tree_priority)
    : has_both_layers(layer_pair.active && layer_pair.pending) {
  if (layer_pair.active) {
    active_queue = layer_pair.active->CreateRasterQueue(
        tree_priority == SMOOTHNESS_TAKES_PRIORITY);
  }

  if (layer_pair.pending) {
    pending_queue = layer_pair.pending->CreateRasterQueue(
        tree_priority == SMOOTHNESS_TAKES_PRIORITY);
  }

  if (has_both_layers)
    SkipTilesReturnedByTwin(tree_priority);

  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
                       "PairedTilingSetQueue::PairedTilingSetQueue",
                       TRACE_EVENT_SCOPE_THREAD, "state", StateAsValue());
}

RasterTilePriorityQueue::PairedTilingSetQueue::~PairedTilingSetQueue() {
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("cc.debug"),
                       "PairedTilingSetQueue::~PairedTilingSetQueue",
                       TRACE_EVENT_SCOPE_THREAD, "state", StateAsValue());
}

bool RasterTilePriorityQueue::PairedTilingSetQueue::IsEmpty() const {
  return (!active_queue || active_queue->IsEmpty()) &&
         (!pending_queue || pending_queue->IsEmpty());
}

Tile* RasterTilePriorityQueue::PairedTilingSetQueue::Top(
    TreePriority tree_priority) {
  DCHECK(!IsEmpty());

  WhichTree next_tree = NextTileIteratorTree(tree_priority);
  TilingSetRasterQueue* next_queue =
      next_tree == ACTIVE_TREE ? active_queue.get() : pending_queue.get();
  DCHECK(next_queue && !next_queue->IsEmpty());
  Tile* tile = next_queue->Top();
  DCHECK(returned_tiles_for_debug.find(tile) == returned_tiles_for_debug.end());
  return tile;
}

void RasterTilePriorityQueue::PairedTilingSetQueue::Pop(
    TreePriority tree_priority) {
  DCHECK(!IsEmpty());

  WhichTree next_tree = NextTileIteratorTree(tree_priority);
  TilingSetRasterQueue* next_queue =
      next_tree == ACTIVE_TREE ? active_queue.get() : pending_queue.get();
  DCHECK(next_queue && !next_queue->IsEmpty());
  DCHECK(returned_tiles_for_debug.insert(next_queue->Top()).second);
  next_queue->Pop();

  if (has_both_layers)
    SkipTilesReturnedByTwin(tree_priority);

  // If no empty, use Top to do DCHECK the next iterator.
  DCHECK(IsEmpty() || Top(tree_priority));
}

void RasterTilePriorityQueue::PairedTilingSetQueue::SkipTilesReturnedByTwin(
    TreePriority tree_priority) {
  // We have both layers (active and pending) thus we can encounter shared
  // tiles twice (from the active iterator and from the pending iterator).
  while (!IsEmpty()) {
    WhichTree next_tree = NextTileIteratorTree(tree_priority);
    TilingSetRasterQueue* next_queue =
        next_tree == ACTIVE_TREE ? active_queue.get() : pending_queue.get();
    DCHECK(next_queue && !next_queue->IsEmpty());

    // Accept all non-shared tiles.
    const Tile* tile = next_queue->Top();
    if (!tile->is_shared())
      break;

    // Accept a shared tile if the next tree is the higher priority one
    // corresponding the iterator (active or pending) which usually (but due
    // to spiral iterators not always) returns the shared tile first.
    if (next_tree == HigherPriorityTree(tree_priority, nullptr, nullptr, tile))
      break;

    next_queue->Pop();
  }
}

WhichTree RasterTilePriorityQueue::PairedTilingSetQueue::NextTileIteratorTree(
    TreePriority tree_priority) const {
  DCHECK(!IsEmpty());

  // If we only have one queue with tiles, return it.
  if (!active_queue || active_queue->IsEmpty())
    return PENDING_TREE;
  if (!pending_queue || pending_queue->IsEmpty())
    return ACTIVE_TREE;

  // Now both iterators have tiles, so we have to decide based on tree priority.
  return HigherPriorityTree(tree_priority, active_queue.get(),
                            pending_queue.get(), nullptr);
}

scoped_refptr<base::debug::ConvertableToTraceFormat>
RasterTilePriorityQueue::PairedTilingSetQueue::StateAsValue() const {
  scoped_refptr<base::debug::TracedValue> state =
      new base::debug::TracedValue();

  bool active_queue_has_tile = active_queue && !active_queue->IsEmpty();
  TilePriority::PriorityBin active_priority_bin = TilePriority::EVENTUALLY;
  TilePriority::PriorityBin pending_priority_bin = TilePriority::EVENTUALLY;
  if (active_queue_has_tile) {
    active_priority_bin =
        active_queue->Top()->priority(ACTIVE_TREE).priority_bin;
    pending_priority_bin =
        active_queue->Top()->priority(PENDING_TREE).priority_bin;
  }

  state->BeginDictionary("active_queue");
  state->SetBoolean("has_tile", active_queue_has_tile);
  state->SetInteger("active_priority_bin", active_priority_bin);
  state->SetInteger("pending_priority_bin", pending_priority_bin);
  state->EndDictionary();

  bool pending_queue_has_tile = pending_queue && !pending_queue->IsEmpty();
  active_priority_bin = TilePriority::EVENTUALLY;
  pending_priority_bin = TilePriority::EVENTUALLY;
  if (pending_queue_has_tile) {
    active_priority_bin =
        pending_queue->Top()->priority(ACTIVE_TREE).priority_bin;
    pending_priority_bin =
        pending_queue->Top()->priority(PENDING_TREE).priority_bin;
  }

  state->BeginDictionary("pending_queue");
  state->SetBoolean("has_tile", active_queue_has_tile);
  state->SetInteger("active_priority_bin", active_priority_bin);
  state->SetInteger("pending_priority_bin", pending_priority_bin);
  state->EndDictionary();
  return state;
}

}  // namespace cc
