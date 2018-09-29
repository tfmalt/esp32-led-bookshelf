#ifndef Effects_h
#define Effects_h


// A typedef to hold a pointer to the current command to execute.
typedef void (*LightCommand)();

class Effects {
    private:
        LightCommand currentCommand;
        LightCommand currentEffect;
        void cmdEmpty();
    public:
        Effects();
};

#endif // Effects_h