#include "Arduino.h"
#include "Battery.h"

void Battery::InitBatteryVoltage()
{
    batteryVoltage[0] = 3.200;
    batteryVoltage[1] = 3.250;
    batteryVoltage[2] = 3.300;
    batteryVoltage[3] = 3.350;
    batteryVoltage[4] = 3.400;
    batteryVoltage[5] = 3.450;
    batteryVoltage[6] = 3.500;
    batteryVoltage[7] = 3.550;
    batteryVoltage[8] = 3.600;
    batteryVoltage[9] = 3.650;
    batteryVoltage[10] = 3.700;
    batteryVoltage[11] = 3.703;
    batteryVoltage[12] = 3.706;
    batteryVoltage[13] = 3.710;
    batteryVoltage[14] = 3.713;
    batteryVoltage[15] = 3.716;
    batteryVoltage[16] = 3.719;
    batteryVoltage[17] = 3.723;
    batteryVoltage[18] = 3.726;
    batteryVoltage[19] = 3.729;
    batteryVoltage[20] = 3.732;
    batteryVoltage[21] = 3.735;
    batteryVoltage[22] = 3.739;
    batteryVoltage[23] = 3.742;
    batteryVoltage[24] = 3.745;
    batteryVoltage[25] = 3.748;
    batteryVoltage[26] = 3.752;
    batteryVoltage[27] = 3.755;
    batteryVoltage[28] = 3.758;
    batteryVoltage[29] = 3.761;
    batteryVoltage[30] = 3.765;
    batteryVoltage[31] = 3.768;
    batteryVoltage[32] = 3.771;
    batteryVoltage[33] = 3.774;
    batteryVoltage[34] = 3.777;
    batteryVoltage[35] = 3.781;
    batteryVoltage[36] = 3.784;
    batteryVoltage[37] = 3.787;
    batteryVoltage[38] = 3.790;
    batteryVoltage[39] = 3.794;
    batteryVoltage[40] = 3.797;
    batteryVoltage[41] = 3.800;
    batteryVoltage[42] = 3.805;
    batteryVoltage[43] = 3.811;
    batteryVoltage[44] = 3.816;
    batteryVoltage[45] = 3.821;
    batteryVoltage[46] = 3.826;
    batteryVoltage[47] = 3.832;
    batteryVoltage[48] = 3.837;
    batteryVoltage[49] = 3.842;
    batteryVoltage[50] = 3.847;
    batteryVoltage[51] = 3.853;
    batteryVoltage[52] = 3.858;
    batteryVoltage[53] = 3.863;
    batteryVoltage[54] = 3.868;
    batteryVoltage[55] = 3.874;
    batteryVoltage[56] = 3.879;
    batteryVoltage[57] = 3.884;
    batteryVoltage[58] = 3.889;
    batteryVoltage[59] = 3.895;
    batteryVoltage[60] = 3.900;
    batteryVoltage[61] = 3.906;
    batteryVoltage[62] = 3.911;
    batteryVoltage[63] = 3.917;
    batteryVoltage[64] = 3.922;
    batteryVoltage[65] = 3.928;
    batteryVoltage[66] = 3.933;
    batteryVoltage[67] = 3.939;
    batteryVoltage[68] = 3.944;
    batteryVoltage[69] = 3.950;
    batteryVoltage[70] = 3.956;
    batteryVoltage[71] = 3.961;
    batteryVoltage[72] = 3.967;
    batteryVoltage[73] = 3.972;
    batteryVoltage[74] = 3.978;
    batteryVoltage[75] = 3.983;
    batteryVoltage[76] = 3.989;
    batteryVoltage[77] = 3.994;
    batteryVoltage[78] = 4.000;
    batteryVoltage[79] = 4.008;
    batteryVoltage[80] = 4.015;
    batteryVoltage[81] = 4.023;
    batteryVoltage[82] = 4.031;
    batteryVoltage[83] = 4.038;
    batteryVoltage[84] = 4.046;
    batteryVoltage[85] = 4.054;
    batteryVoltage[86] = 4.062;
    batteryVoltage[87] = 4.069;
    batteryVoltage[88] = 4.077;
    batteryVoltage[89] = 4.085;
    batteryVoltage[90] = 4.092;
    batteryVoltage[91] = 4.100;
    batteryVoltage[92] = 4.111;
    batteryVoltage[93] = 4.122;
    batteryVoltage[94] = 4.133;
    batteryVoltage[95] = 4.144;
    batteryVoltage[96] = 4.156;
    batteryVoltage[97] = 4.167;
    batteryVoltage[98] = 4.178;
    batteryVoltage[99] = 4.189;
    batteryVoltage[100] = 4.200;
}

Battery::Battery()
{
    InitBatteryVoltage();
}

int Battery::VoltageToPercent(double voltage)
{
    int percent = lastPercent;
    while (true)
    {
        if (voltage > batteryVoltage[100])
        {
            percent = 100;
            break;
        }
        else if (voltage < batteryVoltage[0])
        {
            percent = 0;
            break;
        }

        if (batteryVoltage[percent] < voltage <= batteryVoltage[percent + 1])
        {
            break;
        }
        else if (voltage <= batteryVoltage[percent])
        {
            percent--;
        }
        else
        {
            percent++;
        }
    }
    lastPercent = percent;
    return percent;
}
