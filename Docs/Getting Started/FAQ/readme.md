# PocketMage FAQ

---

#### Helpful Links

[PocketMage.org](https://pocketmage.org/)

[PocketMage - GitHub](https://github.com/ashtf8/PocketMage_PDA)

[PocketMage - Discord](https://discord.gg/KSCapSf4XH)

[Ashtf - YouTube](https://www.youtube.com/@ashtf)

[Ashtf - Patreon](patreon.com/Ashtf)

---

#### General Information

###### **What is PocketMage?**

PocketMage is a hackable PDA device focused on productivity. You can use it to take notes, keep track of tasks, journal, schedule your week, and much more. PocketMage is a hackable revival of the classic PDA-style device. The device features a dual-screen setup with a large E Ink main display and a smaller OLED secondary display. It also has a clamshell design with a clear shell and a full tactile thumb keyboard. PocketMage is fully open-source hardware and software, making it ideal for hacking and add-ons!



###### **How much is the kit?**

The PocketMage final production kit isn't released yet and the price isn't set in stone! That being said, the target price is around the $150 mark for a full kit.



###### **How hard is it to build the kit?**

The kit is designed so that **anyone can build it!** The only tool you need is one allen key, and the kit requires no special skills in software or hardware. There is **no soldering**, **no programming, and no electronics** involved in the build process.



###### **Will I be able to get PocketMage if I live outside the USA?**

We plan to ship PocketMage all around the world!



###### **When can I order?**

There is no set release date for the PocketMage kit at this moment, but once the beta production run is complete and the software has some time to mature, we will move on to the final production! See below for how to get notified when the kit releases!



###### **How can I get notified when the kit releases?**

Visit [PocketMage.org](https://pocketmage.org/) and click the purple "Join the Mailing List!" button to sign up for an email notification.



###### **How can I keep up with the PocketMage project?**

You can subscribe to [Ashtf's YouTube Channel](https://www.youtube.com/@ashtf) for video-style updates, [Ashtf's Patreon](patreon.com/Ashtf) for exclusive weekly written updates, and [The PocketMage Discord Server](https://discord.gg/KSCapSf4XH) for community-driven updates!



---

#### Building and Operating the PocketMage

###### **Where is the build guide?**

The video build guide can be found [here.](https://youtu.be/2aNcC0qCK1o?si=HKqUIyK0ZPkjqzQI)



###### **Is there a guide on the key operations?**

Yes! That guide can be found [here.](https://github.com/ashtf8/PocketMage_PDA/tree/main/Docs/Getting%20Started/Command%20Manual)



###### **How do I flash the firmware?**

First, carefully **remove the battery** out of your PocketMage (pull carefully on the plastic connector). 

Then, ensure the DIP switch for programming mode is switched to the ON position. 

Next, hold the power button while plugging your PocketMage into your computer, **nothing should happen**. 

Next, navigate to [The Web Flasher](https://ashtf8.github.io/PocketMage_PDA/) in a **chrome-based browser** and select your firmware version. The **most recent stable build** (highest number) is recommended. 

If you would like to keep your saved settings, ensure that the "Preserve user data during installation" check box is **checked**. 

Next, hit the flash button and select the PocketMage from the list of USB devices. 

Once the flashing is done, unplug your PocketMage and re-insert the battery. 

PocketMage should boot up automatically!


###### **What SD card can I use?**

You can use any microSD card up to 32gb. **8gb is recommended** since PocketMage files are small and require very little storage. The smaller the SD card you use, the faster the PocketMage will be. SD cards **MUST** be formatted to **FAT32** format.


###### **Where are the CAD files?**

Currently CAD files are available only to Beta Testers. These files will be released to the public alongside the release of the final production PocketMage. If you are a Beta Tester, you can find the password-protected .zip file [here](https://github.com/ashtf8/PocketMage_PDA/tree/main/Docs/CAD) and the password in the beta Discord channel.



###### **Where can I get an extra battery?**

International kits do not come with a battery because of shipping regulations. Battery links for common countries can be found [here.](https://pocketmage.org/get/)



###### **What is the programming mode switch?**

The programming mode switch allows you to program and debug the PocketMage. Internally, it connects the USB port directly to the microcontroller, bypassing the USB and battery charge controller. You may keep programming mode on and use the device as normal with no issues, but your charging will be limited to 500mA as opposed to fast charging when programming mode is turned off.



#### Common Issues

---

###### **My dictionary isn't working!**

The English dictionary isn't installed on the PocketMage out of the box and needs to be downloaded to the SD card. Find the dictionary files [here.](https://github.com/ashtf8/PocketMage_PDA/tree/main/Docs/Dictionary%20(OPTED)) Simply connect your PocketMage to your computer using the USB app and drop all the .txt files into the dictionary folder!



###### **I can't flash the firmware!**

Ensure that: Your battery is unplugged and the programming mode DIP switch is set to ON. Refer to above guide for detailed instructions on flashing.



###### **I found a software bug!**

Please report the bug in detail on the [GitHub issues page](https://github.com/ashtf8/PocketMage_PDA/issues) and include steps to reproduce. We will get to it when we can!



###### **My SD card isn't working!**

First, ensure that your SD card is below 32gb. The recommended size is 8gb, the smaller the SD card the faster the PocketMage will be. Next, ensure that your SD card is formatted to FAT32. It will not work unless it's formatted to FAT32.

