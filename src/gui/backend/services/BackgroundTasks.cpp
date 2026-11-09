#include "backend/services/BackgroundTasks.hpp"

#include <QMetaObject>

#include <exception>
#include <utility>

#include "smm/error.hpp"

namespace smm::gui {

TaskContext::TaskContext(std::shared_ptr<std::atomic_bool> flag, ProgressSink sink)
    : flag_(std::move(flag)), sink_(std::move(sink)) {}

bool TaskContext::cancelled() const {
    return flag_ && flag_->load(std::memory_order_relaxed);
}

void TaskContext::report(int done, int total, const QString& current) {
    if (sink_) {
        sink_(done, total, current);
    }
}

TaskRunner::TaskRunner(QObject* parent)
    : QObject(parent), cancelFlag_(std::make_shared<std::atomic_bool>(false)) {
    pool_.setMaxThreadCount(1);
}

TaskRunner::~TaskRunner() {
    // No worker may outlive the object: queued results posted after this point reference a
    // destroyed receiver and are dropped by Qt instead of being delivered.
    if (cancelFlag_) {
        cancelFlag_->store(true);
    }
    pool_.waitForDone();
}

void TaskRunner::setBusy(bool busy, const QString& label) {
    if (busy_ == busy) {
        return;
    }
    busy_ = busy;
    label_ = busy ? label : QString{};
    emit busyChanged(busy_, label_);
}

bool TaskRunner::run(const QString& label, Job job, Completion completion) {
    if (busy_ || !job) {
        return false;
    }

    cancelFlag_->store(false);
    setBusy(true, label);

    const auto flag = cancelFlag_;
    auto sink = [this](int done, int total, const QString& current) {
        QMetaObject::invokeMethod(
            this,
            [this, done, total, current]() { emit progress(done, total, current); },
            Qt::QueuedConnection);
    };

    pool_.start([this, flag, sink, job = std::move(job),
                 completion = std::move(completion)]() mutable {
        bool ok = true;
        bool wasCancelled = false;
        QString message;
        try {
            TaskContext context(flag, sink);
            job(context);
        } catch (const smm::SmmError& error) {
            ok = false;
            message = QString::fromStdString(error.code_name()) + QStringLiteral(": ") +
                      QString::fromUtf8(error.what());
            if (error.code() == smm::ErrorCode::Cancelled) {
                wasCancelled = true;
                message.clear();
            }
        } catch (const std::exception& error) {
            ok = false;
            message = QString::fromUtf8(error.what());
        } catch (...) {
            ok = false;
            message = QStringLiteral("unknown error");
        }

        QMetaObject::invokeMethod(
            this,
            [this, ok, wasCancelled, message, success = std::move(completion.success),
             failure = std::move(completion.failure),
             cancelled = std::move(completion.cancelled)]() mutable {
                // Deliver first, then drop the busy flag: a handler that queues follow-up
                // work must observe "busy" and be started by busyChanged afterwards, so two
                // results can never be applied out of order.
                if (ok) {
                    if (success) success();
                } else if (wasCancelled && cancelled) {
                    cancelled();
                } else if (failure) {
                    failure(message);
                }
                setBusy(false, {});
            },
            Qt::QueuedConnection);
    });
    return true;
}

void TaskRunner::requestCancel() {
    if (cancelFlag_) {
        cancelFlag_->store(true);
    }
}

bool TaskRunner::cancelRequested() const {
    return cancelFlag_ && cancelFlag_->load(std::memory_order_relaxed);
}

} // namespace smm::gui
