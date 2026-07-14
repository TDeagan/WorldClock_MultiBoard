World Clock v4.1 patch

Replace these files in the working v4.0 project:
  config.h
  01_Hardware.ino
  90_Application.ino

Add these new files:
  02_SplashScreen.ino
  logo.h

The logo is embedded in firmware. The existing deagan_logo_128.png file is
not needed at runtime and may remain in the project folder or be removed.

No SD-card access is used for the splash.

The splash appears immediately after LCD initialization, shows for 3 seconds,
then continues into the unchanged v4.0 startup sequence.
