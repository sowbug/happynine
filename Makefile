DIRS := chrome_app nacl_module
OUTBASE ?= $(PWD)/out
ZIPBASE := $(OUTBASE)/zip
APP_FILES := chrome_app/index.html
ZIP := happynine.zip

all: $(ZIP)

$(ZIP): force_look
	cd chrome_app; ZIPBASE=$(ZIPBASE) $(MAKE) $(MFLAGS)
	cd nacl_module; OUTBASE=$(OUTBASE) ZIPBASE=$(ZIPBASE) $(MAKE) $(MFLAGS)
	rm -rf $(OUTBASE)/$(ZIP)
	cd $(ZIPBASE); zip -r $(OUTBASE)/$(ZIP) .

clean:
	-for d in $(DIRS); do (cd $$d; OUTBASE=$(OUTBASE) $(MAKE) clean ); done
	-@$(RM) -rf $(OUTBASE)

.PHONY: all clean

force_look:
	true
