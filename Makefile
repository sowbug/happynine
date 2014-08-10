# Copyright 2014 Mike Tsao <mike@sowbug.com>

# Permission is hereby granted, free of charge, to any person
# obtaining a copy of this software and associated documentation
# files (the "Software"), to deal in the Software without
# restriction, including without limitation the rights to use, copy,
# modify, merge, publish, distribute, sublicense, and/or sell copies
# of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

PRODUCTNAME ?= Happynine DEV
MAJOR ?= 0
MINOR ?= 0
BUILD ?= 0
PATCH ?= 1

CONFIG ?= Release

DIRS := chrome_app nacl_module
OUTBASE ?= $(PWD)/out
ZIPBASE := $(OUTBASE)/zip
APP_FILES := chrome_app/index.html
ZIP := happynine-$(MAJOR).$(MINOR).$(BUILD).$(PATCH).zip

all: $(ZIP)

$(ZIP): force_look
	mkdir -p $(ZIPBASE)
	cd chrome_app; ZIPBASE=$(ZIPBASE) $(MAKE) $(MFLAGS)
	sed -i'' "s/PRODUCTNAME/${PRODUCTNAME}/" $(ZIPBASE)/manifest.json
	sed -i'' "s/MAJOR/${MAJOR}/" $(ZIPBASE)/manifest.json
	sed -i'' "s/MINOR/${MINOR}/" $(ZIPBASE)/manifest.json
	sed -i'' "s/BUILD/${BUILD}/" $(ZIPBASE)/manifest.json
	sed -i'' "s/PATCH/${PATCH}/" $(ZIPBASE)/manifest.json
	cd nacl_module; \
		OUTBASE=$(OUTBASE) ZIPBASE=$(ZIPBASE) CONFIG=$(CONFIG) $(MAKE) $(MFLAGS)
	rm -rf $(OUTBASE)/$(ZIP)
	cd $(ZIPBASE); zip -r $(OUTBASE)/$(ZIP) .

clean:
	-for d in $(DIRS); do (cd $$d; OUTBASE=$(OUTBASE) $(MAKE) clean ); done
	-@$(RM) -rf $(OUTBASE)

.PHONY: all clean

force_look:
	true
