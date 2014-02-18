ZIPBASE ?= $(PWD)/../out/zip
IMAGES := img

TOOLCHAIN ?= pnacl
CONFIG ?= Release

ifeq ($(WS),1)
CLOSURE_COMPILATION_LEVEL=WHITESPACE_ONLY
else
CLOSURE_COMPILATION_LEVEL=SIMPLE_OPTIMIZATIONS
endif

JS_SOURCES = \
	api_client.js \
	app.js \
	controllers.js \
	credentials.js \
	directives.js \
	electrum.js \
	exporter.js \
	index.js \
	messaging.js \
	node.js \
	retricon.js \
	settings.js \
	sha512.js \
	storage.js \
	utils.js \
	wallet.js \
  #

all: force_look compiled.js
	mkdir -p $(ZIPBASE)/$(TOOLCHAIN)/$(CONFIG)
	cp -R *.html *.json *.css background.js common.js $(ZIPBASE)
	mkdir -p $(ZIPBASE)
	cp -R js $(ZIPBASE)
	mkdir -p $(ZIPBASE)/$(IMAGES)
	cp -R $(IMAGES) $(ZIPBASE)
	rm $(ZIPBASE)/js/bootstrap/js/bootstrap.js
	rm $(ZIPBASE)/js/bootstrap/css/bootstrap.css
	rm $(ZIPBASE)/js/bootstrap/css/bootstrap-theme.css

compiled.js: $(JS_SOURCES)
	java -jar $(GOOGLE_CLOSURE_JAR) \
		--js $(JS_SOURCES) \
		--js js/externs/angular-1.2.js \
		--js js/externs/chrome_extensions.js \
		--js_output_file $(ZIPBASE)/happynine.min.js \
		--create_source_map $(ZIPBASE)/happynine.min.js.map \
		--source_map_format=V3 \
		--language_in ECMASCRIPT5_STRICT \
		--compilation_level $(CLOSURE_COMPILATION_LEVEL)

clean:
	rm -f $(ZIPBASE)/*.html
	rm -f $(ZIPBASE)/*.js
	rm -f $(ZIPBASE)/*.json
	rm -rf $(ZIPBASE)/js
	rm -rf $(ZIPBASE)/$(IMAGES)

force_look:
	true
