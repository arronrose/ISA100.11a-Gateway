Configuring SystemManager project for Eclipse and cygwin

1. build SM standalone
- use buildAccessNodeIsa100.sh to build AccessNode library
- use make file:  make TOOLCHAIN=gcc-linux-pc DEBUG=true TARGET_OSTYPE=linux-pc all

2. running
- check config.ini, sm_subnet.ini
- check log4cpp.properties
- check provisioning in system_manager.ini

5. Enabling Logging on build time
On Properties for SystemManager project (select SystemManager and hit ALT+Enter)
on "C/C++ General" -> Paths and Symbols -> on tab Symbols define:
LOG4CPP_ENABLED = true

6. Code Style configuration
The folder /config contains a file Cpp Code Format 4 SysManager.xml, that contains the code
style that should be used by eclipse.
This file must be loaded in eclipse like this:
1. In menu Window->Preferences
2. tab c/c++ -> Code Style
3. click on import and select the location on dick where the Cpp Code Format 4 SysManager.xml exists.
4. In the combo "Select a profile"  select the "System Manager C++ Style" profile.
This profile will be used on CTRL+SHIFT+F when editing a c++ file, or when r-click on c++ file
and select Source->Format in popupmenu.



