# a07g-exploring-the-CLI

* Team Number: T06
* Team Name: Byte Crafter
* Team Members: Tony Yan & Yue Zhang
* GitHub Repository URL: https://github.com/ese5160/final-project-a07g-a14g-t06-byte-crafter
* Description of test hardware: ROG Zephyrus G14, HUAWEI 14

## 1. Software Architecture

### 1. Revisit Hardware and Software Requirements Specification (HRS & SRS) in A00G

#### 1.1 Hardware Requirements Specification (HRS)

- **HRS 01:** SAMW25 microcontroller shall be used for this project.
- **HRS 02:** LCD display shall be used for user interface with correct information and the refresh rate of the screen should not be too slow (e.g. 3s)
- **HRS 03:** The rotary encoder shall allow the user to navigate the menu on the LCD and select it.
- **HRS 04:** The fingerprint module shall achieve greater than 95% correctness within 1 second.
- **HRS 05:** The IMU shall detect vibrations caused by violent tampering or forced entry attempts.
- **HRS 06:** The buzzer shall be sounded when specific requirements are met.

#### 1.2 Software Requirements Specification (SRS)

- **SRS 01:** The LCD shall have a menu and allow users to add or delete fingerprints, after the master fingerprint has been successfully detected.
- **SRS 02:** The fingerprint module shall open the lock after the authorized fingerprint be detected.
- **SRS 03:** The fingerprint module shall trigger an alarm(buzzer) when it didn't detect authorized fingerprint three times or accelerometer detect someone wants to break the lock.
- **SRS 04:** The system shall send a warning message to cloud when duress fingerprint be detected.
- **SRS 05:** The system shall allow user to view real-time lock status and fingerprint library via cloud.
- **SRS 06:** The system shall allow user to open or close the lock via the cloud.
- **SRS 07:** The system shall allow user to add or delete fingerprint via the cloud.
- **SRS 07:** The system shall lock itself after 5s after open.

### 2. Block diagram outlining the different tasks

Please see below diagram for our project:  
![block_diagram_outlining.png](images/block_diagram_outlining.png)

### Flowcharts or state machine diagram

## 2. Understanding the Starter Code

## 3. Debug Logger Module

## 4. Wiretap the convo

## 5. Complete the CLI

## 6. Add CLI commands

## 7. Using Percepio
