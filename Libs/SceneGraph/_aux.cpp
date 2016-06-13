/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#include "_aux.hpp"
#include <cassert>
#include <stdexcept>

using namespace cf::SceneGraph;


int32_t aux::ReadInt32(std::istream& InFile)
{
    int32_t i;

    InFile.read((char*)&i, sizeof(i));

    return i;
}


uint16_t aux::ReadUInt16(std::istream& InFile)
{
    uint16_t ui;

    InFile.read((char*)&ui, sizeof(ui));

    return ui;
}


uint32_t aux::ReadUInt32(std::istream& InFile)
{
    uint32_t ui;

    InFile.read((char*)&ui, sizeof(ui));

    return ui;
}


float aux::ReadFloat(std::istream& InFile)
{
    float f;

    InFile.read((char*)&f, sizeof(f));

    return f;
}


double aux::ReadDouble(std::istream& InFile)
{
    double d;

    InFile.read((char*)&d, sizeof(d));

    return d;
}


std::string aux::ReadString(std::istream& InFile)
{
    std::string Str;

    while (true)
    {
        char c=0;

        InFile.read(&c, sizeof(c));
        if (c!=0) Str+=c; else break;
    }

    return Str;
}


Vector3dT aux::ReadVector3d(std::istream& InFile)
{
    Vector3T<double> v;

    InFile.read((char*)&v.x, sizeof(v.x));
    InFile.read((char*)&v.y, sizeof(v.y));
    InFile.read((char*)&v.z, sizeof(v.z));

    return v;
}


Vector3fT aux::ReadVector3f(std::istream& InFile)
{
    Vector3T<float> v;

    InFile.read((char*)&v.x, sizeof(v.x));
    InFile.read((char*)&v.y, sizeof(v.y));
    InFile.read((char*)&v.z, sizeof(v.z));

    return v;
}


void aux::Write(std::ostream& OutFile, int32_t i)
{
    OutFile.write((char*)&i, sizeof(i));
}


void aux::Write(std::ostream& OutFile, uint16_t ui)
{
    OutFile.write((char*)&ui, sizeof(ui));
}


void aux::Write(std::ostream& OutFile, uint32_t ui)
{
    OutFile.write((char*)&ui, sizeof(ui));
}


void aux::Write(std::ostream& OutFile, float f)
{
    OutFile.write((char*)&f, sizeof(f));
}


void aux::Write(std::ostream& OutFile, double d)
{
    OutFile.write((char*)&d, sizeof(d));
}


void aux::Write(std::ostream& OutFile, const std::string& Str)
{
    const size_t len=Str.length();
    char c;

    for (size_t i=0; i<len; i++)
    {
        c=Str[i];
        OutFile.write(&c, sizeof(c));
    }

    c=0;
    OutFile.write(&c, sizeof(c));
}


void aux::Write(std::ostream& OutFile, const Vector3T<double>& v)
{
    OutFile.write((char*)&v.x, sizeof(v.x));
    OutFile.write((char*)&v.y, sizeof(v.y));
    OutFile.write((char*)&v.z, sizeof(v.z));
}


void aux::Write(std::ostream& OutFile, const Vector3T<float>& v)
{
    OutFile.write((char*)&v.x, sizeof(v.x));
    OutFile.write((char*)&v.y, sizeof(v.y));
    OutFile.write((char*)&v.z, sizeof(v.z));
}


/*************/
/*** PoolT ***/
/*************/

std::string aux::PoolT::ReadString(std::istream& InFile)
{
    const unsigned long i=ReadUInt32(InFile);

    assert(i<=ReadStrings.Size());

    if (i==ReadStrings.Size())
    {
        ReadStrings.PushBackEmpty();

        while (true)
        {
            char c=0;

            InFile.read(&c, sizeof(c));
            if (c!=0) ReadStrings[i]+=c; else break;
        }
    }

    return ReadStrings[i];
}


Vector3T<double> aux::PoolT::ReadVector3d(std::istream& InFile)
{
    const unsigned long i=ReadUInt32(InFile);

    assert(i<=ReadVectors3d.Size());

    if (i==ReadVectors3d.Size())
    {
        Vector3T<double> v;

        InFile.read((char*)&v.x, sizeof(v.x));
        InFile.read((char*)&v.y, sizeof(v.y));
        InFile.read((char*)&v.z, sizeof(v.z));

        ReadVectors3d.PushBack(v);
    }

    return ReadVectors3d[i];
}


Vector3T<float> aux::PoolT::ReadVector3f(std::istream& InFile)
{
    const unsigned long i=ReadUInt32(InFile);

    assert(i<=ReadVectors3f.Size());

    if (i==ReadVectors3f.Size())
    {
        Vector3T<float> v;

        InFile.read((char*)&v.x, sizeof(v.x));
        InFile.read((char*)&v.y, sizeof(v.y));
        InFile.read((char*)&v.z, sizeof(v.z));

        ReadVectors3f.PushBack(v);
    }

    return ReadVectors3f[i];
}


void aux::PoolT::Write(std::ostream& OutFile, const std::string& s)
{
    std::map<std::string, uint32_t>::const_iterator It=WriteStrings.find(s);

    if (It!=WriteStrings.end())
    {
        // s is already an element of the WriteStrings map.
        uint32_t i=It->second;
        OutFile.write((char*)&i, sizeof(i));
    }
    else
    {
        // s is not yet an element of WriteStrings.
        uint32_t i=aux::cnc_ui32(WriteStrings.size());
        OutFile.write((char*)&i, sizeof(i));

        // WARNING: Writing something like WriteStrings[s]=WriteStrings.size(); here would be a bad mistake!
        // See http://www.parashift.com/c++-faq-lite/serialization.html#faq-36.8 for details.
        WriteStrings[s]=i;
        cf::SceneGraph::aux::Write(OutFile, s);
    }
}


void aux::PoolT::Write(std::ostream& OutFile, const Vector3T<double>& v)
{
    std::map<Vector3dT, uint32_t, LessVector3d>::const_iterator It=WriteVectors3d.find(v);

    if (It!=WriteVectors3d.end())
    {
        // v is already an element of the WriteVectors3d map.
        uint32_t i=It->second;
        OutFile.write((char*)&i, sizeof(i));
    }
    else
    {
        // v is not yet an element of WriteVectors3d.
        uint32_t i=aux::cnc_ui32(WriteVectors3d.size());
        OutFile.write((char*)&i, sizeof(i));

        // WARNING: Writing something like WriteVectors3d[v]=WriteVectors3d.size(); here would be a bad mistake!
        // See http://www.parashift.com/c++-faq-lite/serialization.html#faq-36.8 for details.
        WriteVectors3d[v]=i;

        OutFile.write((char*)&v.x, sizeof(v.x));
        OutFile.write((char*)&v.y, sizeof(v.y));
        OutFile.write((char*)&v.z, sizeof(v.z));
    }
}


void aux::PoolT::Write(std::ostream& OutFile, const Vector3T<float>& v)
{
    std::map<Vector3fT, uint32_t, LessVector3f>::const_iterator It=WriteVectors3f.find(v);

    if (It!=WriteVectors3f.end())
    {
        // v is already an element of the WriteVectors3f map.
        uint32_t i=It->second;
        OutFile.write((char*)&i, sizeof(i));
    }
    else
    {
        // v is not yet an element of WriteVectors3f.
        uint32_t i=aux::cnc_ui32(WriteVectors3f.size());
        OutFile.write((char*)&i, sizeof(i));

        // WARNING: Writing something like WriteVectors3f[v]=WriteVectors3f.size(); here would be a bad mistake!
        // See http://www.parashift.com/c++-faq-lite/serialization.html#faq-36.8 for details.
        WriteVectors3f[v]=i;

        OutFile.write((char*)&v.x, sizeof(v.x));
        OutFile.write((char*)&v.y, sizeof(v.y));
        OutFile.write((char*)&v.z, sizeof(v.z));
    }
}
