/*
 * by @lazykuna, MIT License.
 */

#include "Chart.h"

namespace rparser {

/*
 * @description
 * Song contains all charts using same(or similar) resources.
 * So all charts must be in same folder.
 */
class Song {
private:
    // @description Song object responsive for removing all chart datas when destroyed.
    std::vector<Chart*> m_Charts;
public:
    // @description
    // register chart to Song array.
    // registered chart will be automatically removed from memory when Song object is deleted.
    void RegisterChart(Chart* c);
    // @description
    // delete chart from registered array.
    // you must release object by yourself.
    void DeleteChart(const Chart* c);

    void GetCharts(std::vector<Chart*>& charts);
    int GetChartCount();

    ~Song();
}

};