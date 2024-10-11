this is an implementation of [liads ElectraClimate](https://gist.github.com/liads/c702fd4b8529991af9cd52d03b694814). with a few tweaks and fixes, and as a external component,
since custom components are now being [deprecated](https://esphome.io/guides/contributing#a-note-about-custom-components).
to install add to your yamel config:
```
external_components:
  - source:
      type: git
      url: https://github.com/omersht/ElectraEspHome
      ref: main
```
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
additional options:

Electra AC's(airwell) uses the same command for on/off, but some of these devices also supports a dedicated off command, that will turn the AC off if its on, but wont turn it on if its off, but not all Electra AC's respond to this command.
```
climate:
  - platform: electra
    name: basment ac
    transmitter_id: ir_transmitter
    supports_off_command: True #defaults to True
```
supports_off_command: it is recomended to try it out with true, and only if there is no success change it to False.

to do list:
* add reciving capabilities.(I am trying to figure it out, but with no sucsses, if you can help please make a PR)
