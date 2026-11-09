#pragma once

#include <QObject>
#include <QString>
#include <QThreadPool>

#include <atomic>
#include <functional>
#include <memory>

#include "backend/services/TaskTypes.hpp"

namespace smm::gui {

/// The handle a background job receives: cancellation query plus progress reporting.
///
/// Copies (implicit in the core callbacks) share the same cancellation flag, so a job can
/// hand `cancelled()` to `CoreService` without caring where it runs.
class TaskContext {
public:
    bool cancelled() const;
    void report(int done, int total, const QString& current = {});

private:
    friend class TaskRunner;
    TaskContext(std::shared_ptr<std::atomic_bool> flag, ProgressSink sink);
    std::shared_ptr<std::atomic_bool> flag_;
    ProgressSink sink_;
};

/// Serialized background execution for filesystem work.
///
/// One worker thread and one job at a time on purpose: the staging directory and the game's
/// mods directory are shared mutable resources, so two imports, deploys or renames running
/// concurrently would corrupt each other. Read-only work that must not wait (the doctor probe)
/// gets its own runner instance.
///
/// Lifetime: jobs capture data only. Every result crosses back through a queued call to this
/// object, and `waitForDone()` in the destructor guarantees no worker touches the object while
/// it is being torn down. Deleting the owner therefore cannot leave a dangling `this` running
/// on a worker thread - the failure mode the previous ad-hoc `QThreadPool` calls had.
class TaskRunner : public QObject {
    Q_OBJECT

public:
    using Job = std::function<void(TaskContext&)>;

    /// Where a finished job reports back, always on the GUI thread, never more than once.
    struct Completion {
        std::function<void()> success;
        std::function<void(const QString&)> failure;
        /// Optional: distinguishes a user-requested stop from a real failure.
        std::function<void()> cancelled;
    };

    explicit TaskRunner(QObject* parent = nullptr);
    ~TaskRunner() override;

    bool isBusy() const { return busy_; }
    QString currentLabel() const { return label_; }

    /// Starts `job`. Returns false when a job is already running; false does not queue.
    bool run(const QString& label, Job job, Completion completion);

    /// Asks the running job to stop at its next cancellation point.
    void requestCancel();
    bool cancelRequested() const;

signals:
    void busyChanged(bool busy, const QString& label);
    void progress(int done, int total, const QString& current);

private:
    void setBusy(bool busy, const QString& label);

    QThreadPool pool_;
    std::shared_ptr<std::atomic_bool> cancelFlag_;
    bool busy_{false};
    QString label_;
};

} // namespace smm::gui
