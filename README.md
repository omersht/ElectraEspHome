this is an implementation of [liads ElectraClimate](https://gist.github.com/liads/c702fd4b8529991af9cd52d03b694814). with a few tweaks and fixes, and as a external component,
since custom components are now being [deprecated]([https://pages.github.com/](https://esphome.io/guides/contributing#a-note-about-custom-components)).
this is built over the original [IR Climate Remote](https://esphome.io/components/climate/climate_ir.html), with liads logic, and the tweaks requierd for an electra AC,
so yaml config is the exact same as the [IR Climate Remote](https://esphome.io/components/climate/climate_ir.html), just replace the
```
climate:
  - platform: SOMENAME
```
with:
```
climate:
  - platform: electra
```

to do list:
add reciving capabilities.
