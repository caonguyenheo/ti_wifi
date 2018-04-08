#!/usr/bin/env node

/*
 * ./fota-client.js -version "version" -image "image path" -application "application" -profile "profile" -config "config file path"
 */

var request = require("request");
var fs = require("fs");
var url = require("url");
var argv = require('minimist')(process.argv.slice(2));
var fs = require('fs');
var Dropbox = require("dropbox");

/* checking input */
if (!(argv.version && argv.image && argv.application)) {
  console.error('Fail. Need to provide version, image 1 path, and image 2 path');
  return;
}

var verbose = false;
if (argv.v) {
  verbose = true;
}

/* read configuration */
var config_file = (argv.config)? (argv.config): (process.env.HOME+"/.fotaclient-config.json");
var config_profile = (argv.profile)? (argv.profile): "DEMO";

if (!fs.existsSync(config_file)) {
  console.log('Fail. Check the config file at ', config_file);
  return;
}

var err, data = fs.readFileSync(config_file);

if (err) {
  console.log(err)
  return;
}

var config;
try {
  config = JSON.parse(data);
} catch (e) {
  console.log('Fail. Configuration file ' + config_file + ' is not a JSON');
  return;
}

var HOST = config[config_profile].HOST;
var PROTOCOL = config[config_profile].PROTOCOL;
var APIKEY = config[config_profile].APIKEY;

var image_file = argv.image;
var image_raw = fs.readFileSync(image_file);
var image_name = argv.application+".bin";

/* upload firmwares */
var default_cdn = "DROPBOX";
if (config["CDN"] != default_cdn) {
  console.log('Only support %s as CDN so far', default_cdn);
  return;
}

var dropbox_token = config[default_cdn].KEY;
var dropbox_client = new Dropbox.Client({
  token: dropbox_token
});

dropbox_client.authenticate(function(error, client) {
  if (error) {
    return console.log(error);
  }
  // upload file
  console.log("Uploading to %s ...", default_cdn);
  client.writeFile(image_name, image_raw, function(err, stat) {
    if (err) {
      return console.log("Fail to upload new image ", stat);
    }

    client.makeUrl(image_name, {downloadHack: true}, function(error, status) {
      if (error) {
        return console.log(error);  // Something went wrong.
      }
      DEBUG("Drpobox file status:");
      DEBUG(status);
      console.log("Registering new version ...");
      register_version(status.url);
    })
  })
});


/* register new version with urls */
function register_version(firmware_url) {
  // for registering new version
  var register_version_url = url.format({
      protocol: PROTOCOL,
      host: HOST,
      pathname: "/api/"+argv.application+"/versions"
    })

  var register_version_options = {
    url: register_version_url,
    headers: {
      "api-key": APIKEY,
      "Content-Type": "application/json"
    }
  }
  // backward compatible to esp
  register_version_options.body = JSON.stringify({
    "version": argv.version,
    "firmwares": [
      {name: "image1", url: firmware_url},
      {name: "image2", url: firmware_url},
    ]
  });

  DEBUG("register_version_options");
  DEBUG(register_version_options);


  request.post(register_version_options, function optionalCallback(err, httpResponse, body) {
    if (err) {
      return console.error('Register failed:', err);
    }
    if (httpResponse.statusCode != 201) {
      return console.error("Failed to register new version, make sure you did create new application name ", argv.application);
    }
    console.log('Register application ' + argv.application + ', version ' + argv.version +' successful!');
    console.log('Done!');
  });
}

function DEBUG(message) {
  if (verbose)
    console.log(message)
}