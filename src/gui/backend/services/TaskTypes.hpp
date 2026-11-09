#pragma once

#include <QString>

#include <functional>

namespace smm::gui {

/// Cooperative cancellation asked of a running job.
///
/// Core calls report progress synchronously; the service turns that into "has the user asked
/// to stop?" at a safe point (between files) and throws SmmError(Cancelled) when true.
using CancelToken = std::function<bool()>;

/// Progress tick. Implementations hop to the GUI thread; the core calls this from a worker.
using ProgressSink = std::function<void(int done, int total, const QString& current)>;

} // namespace smm::gui
