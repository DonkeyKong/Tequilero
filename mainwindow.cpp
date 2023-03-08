#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <fstream>
#include <algorithm>

static const int Pump1Line = 26;
static const int Pump2Line = 27;
static const int Pump3Line = 17;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      gpio("/dev/gpiochip0", "Tequilero") {
  ui->setupUi(this);

  timeOfLastTap = std::chrono::system_clock::now();
  timeToDim = std::chrono::minutes(5);
  timeToOff = std::chrono::minutes(60);
  setDisplayBrightness(DisplayPowerState::On, false);

  // Pump setup
  pumps.emplace_back(gpio, Pump1Line, ui->_pump1StartButton, ui->_pump1StopButton, (float)ui->_pump1RateSpinbox->value());
  pumps.emplace_back(gpio, Pump2Line, ui->_pump2StartButton, ui->_pump2StopButton, (float)ui->_pump2RateSpinbox->value());
  pumps.emplace_back(gpio, Pump3Line, ui->_pump3StartButton, ui->_pump3StopButton, (float)ui->_pump3RateSpinbox->value());

  _timer = new QTimer(this);
  connect(_timer, SIGNAL(timeout()), this, SLOT(update()));
  _timer->start(16);
}

MainWindow::Pump::Pump(Gpio& gpioDev, int gpioLine, QPushButton* startButton, QPushButton* stopButton, float flowRate) :
  gpio(gpioDev),
  line(gpioLine),
  startButton(startButton),
  stopButton(stopButton),
  offPending(false),
  flowRate(flowRate)
{
  gpio.setupLine(line, Gpio::LineMode::Output);
  stop();
}

MainWindow::Pump::~Pump()
{
  if (line > 0)
  {
    gpio.releaseLine(line);
  }
}

MainWindow::Pump::Pump(Pump&& other):
  gpio(other.gpio),
  line(other.line),
  startButton(other.startButton),
  stopButton(other.stopButton),
  state(other.state),
  offPending(other.offPending),
  flowRate(other.flowRate),
  offTime(other.offTime)
{
  other.line = -1;
}

void MainWindow::Pump::start(bool scheduleOff, std::chrono::milliseconds duration)
{
  startButton->setEnabled(false);
  stopButton->setEnabled(true);
  gpio.write(line, true);
  offPending = scheduleOff;
  offTime = std::chrono::system_clock::now() + duration;
  state = true;
}

void MainWindow::Pump::dispense(float milliliters)
{
  start(true, std::chrono::milliseconds((int)(milliliters / flowRate * 1000.0f)));
}

void MainWindow::Pump::stop()
{
  startButton->setEnabled(true);
  stopButton->setEnabled(false);
  gpio.write(line, false);
  offPending = false;
  state = false;
}

bool MainWindow::Pump::running()
{
  return state;
}

void MainWindow::Pump::update()
{
  if (offPending && offTime < std::chrono::system_clock::now())
  {
    stop();
  }
}

void setDisplayBacklight(int brightness) {
#ifndef SIMULATE_PI_HARDWARE
  std::ofstream brightnessFile;
  brightnessFile.open("/sys/class/backlight/rpi_backlight/brightness",
                      std::ofstream::trunc);
  brightnessFile << brightness << std::endl;
  brightnessFile.close();

  std::ofstream blPowerFile;
  blPowerFile.open("/sys/class/backlight/rpi_backlight/bl_power");
  blPowerFile << (brightness > 0 ? 0 : 1) << std::endl;
  blPowerFile.close();
#endif
}

MainWindow::~MainWindow() {
  delete ui;

  // Ensure display is on on exit
  setDisplayBacklight(200);
}

void MainWindow::setDisplayBrightness(DisplayPowerState powerState,
                                      bool animate) {
  displayPowerState = powerState;
  if (!animate)
    currentBrightness = (int)powerState;
}

void MainWindow::update()
{
  auto now = std::chrono::system_clock::now();
  // Set the display power state
  if (now - timeOfLastTap > timeToOff)
    setDisplayBrightness(DisplayPowerState::Off);
  else if (now - timeOfLastTap > timeToDim)
    setDisplayBrightness(DisplayPowerState::Dim);
  else
    setDisplayBrightness(DisplayPowerState::On);

  // Crawl towards target brightness
  const int onSpeed = 6;
  const int offSpeed = 2;
  if (currentBrightness < ((int)displayPowerState - onSpeed))
    currentBrightness += onSpeed;
  else if (currentBrightness > ((int)displayPowerState + offSpeed))
    currentBrightness -= offSpeed;
  else
    currentBrightness = (int)displayPowerState;

  setDisplayBacklight(currentBrightness);

  // Pump shutoffs
  bool allPumpsStopped = true;
  bool dispensing = false;
  int msRemaining = -1;
  for (auto& pump : pumps)
  {
    pump.update();
    if (pump.running())
    {
      allPumpsStopped = false;

      if (pump.offPending)
      {
        dispensing = true;
        msRemaining = std::max(msRemaining, (int)std::chrono::duration_cast<std::chrono::milliseconds>(pump.offTime - now).count());
      }
    }
  }

  if (allPumpsStopped)
  {
    ui->_pourCustomDrinkButton->setEnabled(true);
  }

  if (dispensing)
  {
    ui->statusLabel->setText("Auto Pour");
    ui->timeRemainingLabel->setText(QString::number((float)msRemaining / 1000.0f, 'f', 1));
  }
  else if (allPumpsStopped)
  {
    ui->statusLabel->setText("Idle");
    ui->timeRemainingLabel->setText("N/A");
  }
  else
  {
    ui->statusLabel->setText("Manual Pour");
    ui->timeRemainingLabel->setText("N/A");
  }
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
  // Intercept all mouse click events and use them to
  // update the last tap timer
  if (event->type() == QEvent::MouseButtonPress ||
      event->type() == QEvent::MouseMove) {
    timeOfLastTap = std::chrono::system_clock::now();

    // Additionally, if the display is currently off,
    // then eat the click event to prevent accidental operation
    if (displayPowerState == DisplayPowerState::Off)
      return true;
  }
  return QMainWindow::eventFilter(watched, event);
}

void MainWindow::on__pump1StartButton_clicked()
{
  pumps[0].start();
  ui->_pourCustomDrinkButton->setEnabled(false);
}

void MainWindow::on__pump2StartButton_clicked()
{
  pumps[1].start();
  ui->_pourCustomDrinkButton->setEnabled(false);
}

void MainWindow::on__pump3StartButton_clicked()
{
  pumps[2].start();
  ui->_pourCustomDrinkButton->setEnabled(false);
}


void MainWindow::on__pump1StopButton_clicked()
{
  pumps[0].stop();
}


void MainWindow::on__pump2StopButton_clicked()
{
  pumps[1].stop();
}


void MainWindow::on__pump3StopButton_clicked()
{
  pumps[2].stop();
}

void MainWindow::on__stopAllButton_clicked()
{
  for (auto& pump : pumps)
  {
    pump.stop();
  }
}


void MainWindow::on__pourCustomDrinkButton_clicked()
{
  pumps[0].dispense((float)ui->_pump1PourSpinbox->value());
  pumps[1].dispense((float)ui->_pump2PourSpinbox->value());
  pumps[2].dispense((float)ui->_pump3PourSpinbox->value());
  ui->_pourCustomDrinkButton->setEnabled(false);
}

void MainWindow::on__pump1RateSpinbox_valueChanged(double arg1)
{
    pumps[0].flowRate = (float)arg1;
}


void MainWindow::on__pump2RateSpinbox_valueChanged(double arg1)
{
    pumps[1].flowRate = (float)arg1;
}


void MainWindow::on__pump3RateSpinbox_valueChanged(double arg1)
{
    pumps[2].flowRate = (float)arg1;
}
