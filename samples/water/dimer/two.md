#include "water.md"

nComponents = 1;
component{
  type = "TIP4P";
  nMol = 2;
}

initialConfig = "./two.in";


ensemble = NVT;
forceField = "WATER";
useReactionField = "false";
dielectric = 80.0;
cutoffRadius = 9.0;
switchingRadius = 7.8;

density = 0.0334;

targetTemp = 0.001;
targetPressure = 1.0;

tauThermostat = 1e4;
tauBarostat = 1e4;

dt = 1.0;
runTime = 1e5;

sampleTime = 3e4;
statusTime = 1;
useInitialTime = "false";
useInitialExtendedSystemState = "false";