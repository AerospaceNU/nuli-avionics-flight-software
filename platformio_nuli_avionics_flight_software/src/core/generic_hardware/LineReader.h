#ifndef LINEREADER_H
#define LINEREADER_H

class LineReader {
public:
    virtual ~LineReader() = default;

    virtual bool readLine();

    virtual char *getLine();
};

#endif //LINEREADER_H
