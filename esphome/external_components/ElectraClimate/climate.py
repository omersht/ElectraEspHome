import esphome.config_validation as cv
import esphome.codegen as cg

# Define your configuration keys
CONF_TRANSMITTER_ID = 'transmitter_id'
CONF_TEMP_SENSOR_ID = 'temp_sensor_id'

# Define the configuration schema
CONFIG_SCHEMA = cv.Schema({
    cv.Required(CONF_TRANSMITTER_ID): cv.use_id(Remote),  # A required key that must be a string  # An optional key with a default value
}).extend(cv.COMPONENT_SCHEMA)  # Extend to include the base component schema


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    cg.add(var.set_my_required_key(config[CONF_MY_REQUIRED_KEY]))