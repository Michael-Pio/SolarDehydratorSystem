class PresetValues {
public:
    // Constructor to initialize preset values
    PresetValues() {
        // Initialize default values for each mode
        for (int i = 0; i < 10; i++) { // Index 0 is not used
            modes[i].tresTemp = 0;
            modes[i].tresHumid = 0;
        }
        
        // Example preset values for modes 1 to 9
        modes[1] = {25, 50}; // Preset values for Mode 1
        modes[2] = {30, 55}; // Preset values for Mode 2
        modes[3] = {28, 60}; // Preset values for Mode 3
        modes[4] = {26, 53}; // Preset values for Mode 4
        modes[5] = {27, 57}; // Preset values for Mode 5
        modes[6] = {29, 62}; // Preset values for Mode 6
        modes[7] = {24, 48}; // Preset values for Mode 7
        modes[8] = {23, 45}; // Preset values for Mode 8
        modes[9] = {22, 40}; // Preset values for Mode 9
    }

    struct ModeValues {
        int tresTemp;
        int tresHumid;
    };

    ModeValues modes[10]; // Array to store values for modes 1 to 9

    // Function to get preset values for a specific mode
    ModeValues getValues(int mode) {
        if (mode < 1 || mode > 9) {
            // Return defaults if mode is out of range
            return {0, 0};
        }
        return modes[mode];
    }
};
