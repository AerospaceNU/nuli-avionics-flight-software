import pandas as pd

# Standard atmosphere defaults (override if you have project-specific constants)
LAPSE_RATE_K_M = 0.0065        # K/m
ATMOSPHERIC_PRESSURE_PA = 101325.0  # Pa
GAS_CONSTANT_J_KG_K = 287.05   # J/(kg·K) (specific gas constant for dry air)
G_EARTH_MSS = 9.80665          # m/s^2

def calculate_altitude_m(pressure_pa, temperature_k,
                         lapse_rate_k_m=LAPSE_RATE_K_M,
                         atmospheric_pressure_pa=ATMOSPHERIC_PRESSURE_PA,
                         gas_constant_j_kg_k=GAS_CONSTANT_J_KG_K,
                         g_earth_mss=G_EARTH_MSS):
    """
    Calculate altitude (meters) from pressure (Pa) and temperature (K)
    using the barometric formula and a linear lapse rate.

    Args:
        pressure_pa (float): pressure in pascals
        temperature_k (float): temperature in kelvin
        lapse_rate_k_m (float): temperature lapse rate in K/m (default 0.0065)
        atmospheric_pressure_pa (float): sea-level reference pressure in Pa (default 101325)
        gas_constant_j_kg_k (float): specific gas constant for air (default 287.05)
        g_earth_mss (float): gravity (default 9.80665)

    Returns:
        float: altitude in meters
    """
    return (temperature_k / lapse_rate_k_m) * (
        (pressure_pa / atmospheric_pressure_pa) ** (-gas_constant_j_kg_k * lapse_rate_k_m / g_earth_mss)
        - 1.0)

filename = 'simulation/data/staticstilldataoffload_20251202_095151_flight2.txt'
still_df = pd.read_csv(filename,sep=None)

still_df['barometerAltitude'] = calculate_altitude_m(still_df['pressurePa'], still_df['barometerTemperatureK'])

x_var, y_var, z_var = still_df['accelerationMSS_x'].var(), still_df['accelerationMSS_y'].var(), still_df['accelerationMSS_z'].var()
avg_var = (x_var + y_var + z_var) / 3

bar_temp = still_df['barometerTemperatureK'].mean()
bar_pa_var, bar_pa_std, bar_pa_mean = still_df['pressurePa'].var(), still_df['pressurePa'].std(), still_df['pressurePa'].mean()
alt_var = still_df['unfilteredAltitudeM'].var()
bar_alt_var = still_df['barometerAltitude'].var()
avg_alt_from_pa_temp = calculate_altitude_m(bar_pa_mean, bar_temp)
print(f'Acceleration X, Y, Z variances while standing still: {x_var}, {y_var}, {z_var}\nWith an average variance of: {avg_var}')

print(f'''
Avg bar temp: {bar_temp}
Avg barometer pressure: {bar_pa_mean}
Altitude at average temp and pressure: {avg_alt_from_pa_temp}
Altitude at average temp and +13 Pa: {calculate_altitude_m((bar_pa_mean + bar_pa_std), bar_temp)}
Datasheet altitude variance: {(avg_alt_from_pa_temp - calculate_altitude_m((bar_pa_mean + bar_pa_std), bar_temp))**2}
Barometer pressure variance: {bar_pa_var}
Barometer pressure std dev: {bar_pa_std}
Unfiltered altitude variance: {alt_var}
Barometer altitude variance: {bar_alt_var}
''')

