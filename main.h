void setupRESTServer();
void handleRequest();
void sendStatus();
void setupWifi();
void parseGetRequest();
double average(unsigned int startIncl, unsigned int endExcl);
void analyzeData();
void notifyListeners();
void setTarget(int t);
void stop();
void inflate();
void deflate();
void sendWebpage();

enum Mode
{
    TARGET,
    INFLATE,
    DEFLATE
};