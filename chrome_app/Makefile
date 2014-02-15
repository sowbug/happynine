ZIPBASE ?= $(PWD)/../out/zip
IMAGES := img

TOOLCHAIN ?= pnacl
CONFIG ?= Release

JS_SOURCES = \
	app.js \
	controllers.js \
	credentials.js \
	directives.js \
	electrum.js \
	index.js \
	messaging.js \
	node.js \
	settings.js \
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
		--js_output_file $(ZIPBASE)/compiled.js \
		--language_in ECMASCRIPT5_STRICT \
		--compilation_level SIMPLE_OPTIMIZATIONS

clean:
	rm -f $(ZIPBASE)/*.html
	rm -f $(ZIPBASE)/*.js
	rm -f $(ZIPBASE)/*.json
	rm -rf $(ZIPBASE)/js
	rm -rf $(ZIPBASE)/$(IMAGES)

force_look:
	true
