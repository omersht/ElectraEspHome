this is an implementation of [liads ElectraClimate](https://gist.github.com/liads/c702fd4b8529991af9cd52d03b694814). with receiving capabilities a few tweaks and fixes, and as a external component,
since custom components are now being [deprecated](https://esphome.io/guides/contributing#a-note-about-custom-components).
to install add to your yaml config:
```
external_components:
  - source:
      type: git
      url: https://github.com/omersht/ElectraEspHome
      ref: main
```
this is built over the original [IR Climate Remote](https://esphome.io/components/climate/climate_ir.html), with sending liads logic, and the tweaks requierd for an electra AC,
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
    receiver_id: ir_receiver
    id: someid
    supports_off_command: True #defaults to True
```
supports_off_command: it is recomended to try it out with true, and only if there is no success change it to False.

if you wish to be able to sync the state of the ac, (change saved state without sending any ir code) using an Home Assistant service call, add this to your yaml;
```
api:
  actions:
    - action: sync_electra_state
      then:
        - lambda: |-
            id(someid).sync_state();
```
change the id to the id you set up in the climate declartion(step above).
then you can call this in homeassistant with:
action: esphome.YourDeviceName_sync_electra_state
replace the YourDeviceName with the name definde under:
```
esphome:
  name: YourDeviceName
```

to do list:
* create an home assistant blueprint, to enable the use of contact sensors on the ac to report the actual state
