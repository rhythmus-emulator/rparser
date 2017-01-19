/*
 * by @lazykuna, MIT License.
 */

namespace rparser {

/*
 * @description
 * contains special channel objects, like BPM/STOP
 * and calculates real-time position by these information.
 */
class TimingData {
    public:
    // @description
    // stores measure length(double) for measure index(int).
    // default measure length is 1, in case of size non-designated measure. 
    std::map<int, double> measureLength;
    
    private:
};

}