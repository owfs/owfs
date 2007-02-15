CSocketMaster 1.1 readme file
=============================

*** What is CSocketMaster?
==========================

CSocketMaster class is a Winsock control substitute that attempts to mimic it's behavior and interface to make it easy to implement.

*** How do you use it?
======================

1) Add CSocketMaster.cls and modSocketMaster.bas to your project.
2) In a Form, Class or UserControl add this line in the declaration area for each socket you want to use:
 
Dim WithEvents NameOfSocket As CSocketMaster

where 'NameOfSocket' is any name you want for the socket.

3) Finally you need to create the object before using it. Add this line in a sub or funtion:

Set NameOfSocket = New CSocketMaster

That's it.

*** Differences between CSocketMaster and Winsock control
=========================================================

A) Winsock Close function is CloseSck in CSocketMaster.
B) Winsock Close event is CloseSck in CSocketMaster.
C) WndProc function is used to deliver system messages and you should not call it under any circumstance.
D) There are some other differences I intentionally put in CSocketMaster that you shouldn't notice. If you find a major difference that you think shouldn't be there, please let me know.

*** Known issues
================

A) Msgbox
Due to a VB behavior by desing running a project in the IDE that displays a message box created with Msgbox prevents events from occurring. If you really need to use message boxes then you can use  MessageBox api instead. Note that this only happens in the IDE, when you compile your project all events are fired no matter what. For more info check this site:
 http://support.microsoft.com/default.aspx?scid=kb;en-us;178078


*** Why not use Winsock control?
================================

Bacause winsock it's a fixed non-modifiable control. Also it is known that Winsock has a memory leak.


*** Why use a class instead of a control?
=========================================

Using a class instead of a control has some clear benefits. You have total encapsulation, and you could, for example, make a class (or control) that downloads files from internet just passing a URL without knowing how CSocketMaster works internally. You can modify CSocketMaster and add more fancy functions and events. Best of all: you don't have external dependencies.


*** Why use a control instead of a class?
=========================================

There's a Winsock control clone as part of the sample projects. With a control you can build arrays and create sockets at runtime easier.


*** What tha' heck is that subclassing stuff?
=============================================

When you work with winsock you need to subclass a window to receive system's info from the socket. If you are familiar with the concept of subclassing on VB you should know that if you subclass a window and press then End button while in IDE you get a nice GPF and your system crashes. To solve this I use a code based on Paul Caton's WinSubHook2 that can be found at www.planet-source-code.com. If you wanna understand how my subclassing code works you have to understand his first. The asm code for the WndProc is in Subclass.asm file. This subclassing approach is not fully tested yet, so let me know if you get a GPF.


*** Why so many Debug logs?
===========================

That's for helping me (and you) to find any possible bug. If you get annoyed you can always erase them or comment them.


*** Bug report
==============

If you have a question firs't check this site:

 http://www.geocities.com/anshoku/

I will post FAQs to this page so I don't have to answer a question twice.
If you don't find your answer there you can send me an email, but first read this:

A) If I consider that your question is answered on my page your email will be ignored.
B) Make sure it's a bug! Maybe what you think it's a bug is normal winsock behavior. Use winsock control to check if the malfunction persists.
C) I suppose you know how to work with winsock cause I will not teach you how to do it. So don't ask me something like 'how to make a connection?' or 'how can I make a chat prog?'.
D) If you could send me some sample code to illustrate the bug, that would be great.
E) Don't be a lamer, don't send spam or chain mails.

Emiliano Scavuzzo <anshoku@yahoo.com>
Rosario, Argentina

