#include "message_pump_qt.h"

#include <QApplication>

void MessagePumpQt::Run(Delegate* delegate) {
  RunState s;
  s.delegate = delegate;
  s.should_quit = false;
  s.run_depth = state_ ? state_->run_depth + 1 : 1;

  RunState* previous_state = state_;
  state_ = &s;

  if (s.run_depth == 1) {
    QApplication::exec();
  } else {
    state_->event_loop = new QEventLoop;
    state_->event_loop->exec();
  }
  state_ = previous_state;
}

void MessagePumpQt::Quit() {
  state_->should_quit = true;
  if (state_->event_loop) {
    state_->event_loop->quit();
  } else {
    QApplication::quit();
  }
}

void MessagePumpQt::ScheduleWork() {
  // kHaveWork
  QCoreApplication::postEvent(
      this, new QEvent(static_cast<QEvent::Type>(kHaveEvent)));
}

void MessagePumpQt::ScheduleDelayedWork(
    const base::TimeTicks& delayed_work_time) {
  delayed_work_time_ = delayed_work_time;
  RescheduleTimer();
}

void MessagePumpQt::customEvent(QEvent* event) {
  if (event->type() == kHaveEvent) {
    if (state_->delegate->DoWork())
      ScheduleWork();
    state_->delegate->DoDelayedWork(&delayed_work_time_);
    RescheduleTimer();
  }
}

void MessagePumpQt::timerEvent(QTimerEvent* event) {
  killTimer(timer_id_);
  timer_id_ = -1;

  if (!state_)
    return;

  state_->delegate->DoDelayedWork(&delayed_work_time_);
  RescheduleTimer();
}

void MessagePumpQt::RescheduleTimer() {
  if (delayed_work_time_.is_null())
    return;

  int delay_msec = GetCurrentDelay();
  if (delay_msec == 0) {
    ScheduleWork();
  } else {
    if (timer_id_ != -1) {
      killTimer(timer_id_);
    }
    timer_id_ = startTimer(std::chrono::milliseconds(delay_msec));
  }
}

int MessagePumpQt::GetCurrentDelay() const {
  if (delayed_work_time_.is_null())
    return -1;

  // Be careful here.  TimeDelta has a precision of microseconds, but we want a
  // value in milliseconds.  If there are 5.5ms left, should the delay be 5 or
  // 6?  It should be 6 to avoid executing delayed work too early.
  double timeout =
      ceil((delayed_work_time_ - base::TimeTicks::Now()).InMillisecondsF());

  // Range check the |timeout| while converting to an integer.  If the
  // |timeout| is negative, then we need to run delayed work soon.  If the
  // |timeout| is "overflowingly" large, that means a delayed task was posted
  // with a super-long delay.
  return timeout < 0 ? 0
                     : (timeout > std::numeric_limits<int>::max()
                            ? std::numeric_limits<int>::max()
                            : static_cast<int>(timeout));
}

