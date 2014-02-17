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
