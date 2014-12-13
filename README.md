TempLang - Temporary switch active keyboard layout
==================================================

Anyone who speaks two or more languages, and in particular, languages that have different scripts, is familiar with this
problem. You type a sentence in one language, and then want to type just a word or two in the another. The keyboard
sequence on Windows to achieve this is fairly long. You need to release all keys, depress the left Alt, press the shift
key, release both keys. Now you can type your alternate language text (sometimes just a thre letter acronym, or TLA),
and now repeat the Alt-Shift combination. While this doesn't sound like much, it totally breaks the flow of typing.

At one and the same time, any keyboard has lots of keys nobody at all uses any more. The most obvious one is, of course,
the Caps-Lock key.

TempLang does something very simple. It converts the Caps-Lock key from its current, useless, function to a special shift
key (non-locking) that simply switches the active language _for the duration the key is pressed_. As soon as the key is
released, the active language resorts to the original language.

Compilation Requirements
========================

To compile the project you will need Visual Studio 2013 Community Edition. To create an installable image, you will need
Microsoft Visual Studio Installer Projects. Both are available, free of charge, from Microsoft.

http://www.visualstudio.com/en-us/downloads/download-visual-studio-vs#d-community
https://visualstudiogallery.msdn.microsoft.com/9abe329c-9bba-44a1-be59-0fbf6151054d