struct TimeStretch
{
    //udpates g = grainsize, l = target len (aka stretch factor), comp = pitch compensation
    void update(float g, float l, bool comp)
    {
        //float pitch compensator, so 200% (2.0) means 2x speed means +12 semitones
        pitchCompensator = comp ? l : 1;
        stretchComp = comp;
        grainSize = g;
        stretchFactor = l;

        //various other factors we need, could do with better names
        gInv = 1.0 / grainSize;
        m = 1 / (grainSize * c);
        
        f1 = grainSize * c;
        f2 = grainSize * cPrime;
        
        stretchFactorInverse = 1 / stretchFactor;
        
        grainOffset = grainSize*cPrime*stretchFactorInverse;
    }
    
    //should be called to reset the state of the grains
    void reset(double samplePos, int direction) {
        if(direction==1){
            grainPlaying = 0;
            
            phase[0] = 0;
            phase[1] = -1;
            
            grain[0] = samplePos;
            grain[1] = samplePos;
            totalPhase = samplePos;
        }
        else{
            grainPlaying = 0;
            
            phase[0] = grainSize;
            phase[1] = -1;
            
            grain[0] = samplePos - grainSize;
            grain[1] = samplePos - grainSize;
            totalPhase = samplePos;
        }
    }
    
    void loop(double loopPosition, int direction) {
        bool reverse = direction ==-1;
        
        if(reverse) {
            grain[!grainPlaying] = loopPosition - grainSize;
            phase[!grainPlaying] = grainSize;
        }
        else {
            grain[!grainPlaying] = loopPosition;
            phase[!grainPlaying] = 0;
        }
        grainPlaying = !grainPlaying;
        totalPhase = loopPosition;
    }
    
    //advances the timestretch, should be done once every sample
    double advance(double pitchDelta, double sampleStart, double sampleEnd, int direction)
    {
        pitchDelta *= pitchCompensator;
        bool reverse = direction ==-1;
        if(!reverse){
            totalPhase += pitchDelta*stretchFactorInverse;
            
            if (grain[grainPlaying] + phase[grainPlaying] < sampleEnd)
                phase[grainPlaying] += pitchDelta;


            if(phase[!grainPlaying] > -1 && grain[!grainPlaying] + phase[!grainPlaying] < sampleEnd)
                phase[!grainPlaying] += pitchDelta;
            
            
            if(phase[!grainPlaying] >= grainSize) {
                phase[!grainPlaying] = -1;
            }
            
            if(phase[grainPlaying] >= f2){
                phase[!grainPlaying] = 0;
                grain[!grainPlaying] = grain[grainPlaying] + grainOffset;
                grainPlaying =  1 - grainPlaying;
            }
        }
        else{
            //REVERSED PLAYING
            totalPhase -= pitchDelta * stretchFactorInverse;

            if(grain[grainPlaying] + phase[grainPlaying] > sampleStart)
                phase[grainPlaying] -= pitchDelta;

            if(phase[!grainPlaying] > -1 && grain[!grainPlaying] + phase[!grainPlaying] > sampleStart) 
                phase[!grainPlaying] -= pitchDelta;
            
            if(phase[!grainPlaying] <= 0) {
                phase[!grainPlaying] = -1;
            }
            
            if(phase[grainPlaying] <= f1){
                phase[!grainPlaying] = grainSize;
                grain[!grainPlaying] = grain[grainPlaying] - grainOffset;
                grainPlaying = 1 - grainPlaying;
            }
        }
        return totalPhase;
    }
    
    //use a generic interpolation lambda as your interpolation function, can be removed if desired.
    float get(float *data, std::function<float(double, float*)>& interp, int direction) {
        //if both grains are active
        if(phase[0] != -1 && phase[1] != -1){
            bool reverse = (direction == -1);
            auto phi = !reverse ? phase[grainPlaying] : grainSize - phase[grainPlaying];

            //linear window
            double window = m*phi;
            
            double x0 = grain[grainPlaying] + phase[grainPlaying];
            double x1 = grain[!grainPlaying] + phase[!grainPlaying];
            return interp(x0, data)*(window) + interp(x1, data)*(1-window);
        }
        else{
            double x = grain[grainPlaying] + phase[grainPlaying];
            return interp(x, data);
        }
    }
    
    /*
    two grains playing at once,
    "grainPlaying" is the main grain that is currently playing and swap from 0 to 1 by continually calling advance(),
    "grain" indicates the start of the grain for grainPlaying and !grainPlaying
    "phase" is the current phase for the two grains, in samples. so 0 to grainSize.
    grainOffset is how much the next grain gets offset. goes from 0 to grainSize * (1.0 - c)   //where c is crossfade amount
    */
    double phase[2] = {0, 0};
    double grain[2] = {0, 0};
    double totalPhase = 0;
    double grainOffset = 0;
    int grainPlaying = 0;
    
    //parameters
    double stretchFactor = 1;
    double grainSize = 1;
    bool stretchComp = false;
    
    //derived from parameters
    double pitchCompensator = 1;
    double stretchFactorInverse = 1;
        
    double gInv = 1.0 / grainSize;
    //CROSSFADE AMOUNT
    double c = 0.4;
    double cPrime = (1.0 - c);
    double cInv = 1.0/c;
    
    double m = 1 / (grainSize * c);
    
    //fade point one and fade point two
    double f1 = grainSize * c;
    double f2 = grainSize * cPrime;
};