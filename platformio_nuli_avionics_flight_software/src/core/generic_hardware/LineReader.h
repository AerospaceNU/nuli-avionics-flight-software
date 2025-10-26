#ifndef LINEREADER_H
#define LINEREADER_H

class LineReader {
public:
    virtual ~LineReader() = default;

    virtual bool readLine() = 0;

    virtual char* getLine() = 0;
};

#endif //LINEREADER_H
