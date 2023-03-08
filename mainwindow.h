#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QPushButton>
#include <gpio-cpp/gpio.hpp>
#include <chrono>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    enum class DisplayPowerState : int
    {
      On = 200,
      Dim = 30,
      Off = 0
    };
    
    bool eventFilter(QObject * watched, QEvent * event) override;

private:
    void setDisplayBrightness(DisplayPowerState powerState, bool animate = true);

private slots:
    void update();

    void on__pump1StartButton_clicked();

    void on__pump2StartButton_clicked();

    void on__pump3StartButton_clicked();

    void on__pump1StopButton_clicked();

    void on__pump2StopButton_clicked();

    void on__pump3StopButton_clicked();

    void on__stopAllButton_clicked();

    void on__pourCustomDrinkButton_clicked();

    void on__pump1RateSpinbox_valueChanged(double arg1);

    void on__pump2RateSpinbox_valueChanged(double arg1);

    void on__pump3RateSpinbox_valueChanged(double arg1);

private:
  Ui::MainWindow *ui;
    QTimer* _timer;
    int currentBrightness;
    Gpio gpio;
    
    std::chrono::system_clock::time_point timeOfLastTap;
    std::chrono::system_clock::duration timeToDim;
    std::chrono::system_clock::duration timeToOff;
    DisplayPowerState displayPowerState;

    struct Pump
    {
      Pump(Gpio& gpioDev, int gpioLine, QPushButton* startButton, QPushButton* stopButton, float flowRate);
      ~Pump();
      Pump(const Pump&) = delete;
      Pump(Pump&& other);
      void start(bool scheduleOff = false, std::chrono::milliseconds duration = std::chrono::milliseconds(0));
      void dispense(float milliliters);
      void stop();
      void update();
      bool running();
      Gpio& gpio;
      int line;
      QPushButton* startButton;
      QPushButton* stopButton;
      bool state;
      bool offPending;
      float flowRate;
      std::chrono::system_clock::time_point offTime;
    };

    std::vector<Pump> pumps;
};
#endif // MAINWINDOW_H
