#ifndef MESSAGE_PUMP_QT_H_
#define MESSAGE_PUMP_QT_H_

#include <QEvent>
#include <QEventLoop>
#include <QObject>

#include <base/message_loop/message_pump.h>

class MessagePumpQt : public QObject, public base::MessagePump {
  Q_OBJECT
 public:
  // MessagePump methods:
  void Run(Delegate* delegate) override;
  void Quit() override;
  void ScheduleWork() override;
  void ScheduleDelayedWork(const base::TimeTicks& delayed_work_time) override;

  // QObject methods:
  void customEvent(QEvent* event) override;
  void timerEvent(QTimerEvent* event) override;

 private:
  void RescheduleTimer();
  int GetCurrentDelay() const;
  int kHaveEvent = QEvent::registerEventType(QEvent::User);
  int timer_id_{-1};

  struct RunState {
    Delegate* delegate;

    // Used to flag that the current Run() invocation should return ASAP.
    bool should_quit;

    QEventLoop* event_loop;

    // Used to count how many Run() invocations are on the stack.
    int run_depth;
  };

  // State for the current invocation of Run.
  RunState* state_ = nullptr;

  // The time at which delayed work should run.
  base::TimeTicks delayed_work_time_;
};

#endif  // MESSAGE_PUMP_QT_H_