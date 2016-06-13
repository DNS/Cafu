/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

#ifndef CAFU_MAP_PLANT_HPP_INCLUDED
#define CAFU_MAP_PLANT_HPP_INCLUDED

#include "MapPrimitive.hpp"
#include "Plants/Tree.hpp"


class  MapDocumentT;
struct PlantDescriptionT;
class  PlantDescrManT;


class MapPlantT : public MapPrimitiveT
{
    public:

    MapPlantT();
    MapPlantT(const PlantDescriptionT* PlantDescription, unsigned long RandomSeed, const Vector3fT& Position);

    /// The copy constructor for copying a plant.
    /// @param Plant   The plant to copy-construct this plant from.
    MapPlantT(const MapPlantT& Plant);


    // Implementations and overrides for base class methods.
    MapPlantT* Clone() const override;


    // MapElementT implementation.
    BoundingBox3fT GetBB() const;

    void Render2D(Renderer2DT& Renderer) const;
    void Render3D(Renderer3DT& Renderer) const;

    bool TracePixel(const wxPoint& Pixel, int Radius, const ViewWindow2DT& ViewWin) const;
    wxString GetDescription() const { return "Plant"; }
    bool IsTranslucent() const { return true; }

    // Implement the MapElementT transformation methods.
    TrafoMementoT* GetTrafoState() const override;
    void RestoreTrafoState(const TrafoMementoT* TM) override;
    void TrafoMove(const Vector3fT& Delta, bool LockTexCoords) override;
    void TrafoRotate(const Vector3fT& RefPoint, const cf::math::AnglesfT& Angles, bool LockTexCoords) override;
    void TrafoScale(const Vector3fT& RefPoint, const Vector3fT& Scale, bool LockTexCoords) override;
    void TrafoMirror(unsigned int NormalAxis, float Dist, bool LockTexCoords) override;
    void Transform(const MatrixT& Matrix, bool LockTexCoords) override;

    void Load_cmap(TextParserT& TP, MapDocumentT& MapDoc, bool IgnoreGroups) override;
    void Save_cmap(std::ostream& OutFile, unsigned long PlantNr, const MapDocumentT& MapDoc) const;

    // The TypeSys related declarations for this class.
    virtual const cf::TypeSys::TypeInfoT* GetType() const { return &TypeInfo; }
    static void* CreateInstance(const cf::TypeSys::CreateParamsT& Params);
    static const cf::TypeSys::TypeInfoT TypeInfo;


    private:

    friend class InspDlgPrimitivePropsT;
    friend class CommandChangePlantSeedT;
    friend class CommandChangePlantDescrT;

    TreeT              m_Tree;
    unsigned long      m_RandomSeed;
    cf::math::AnglesfT m_Angles;
    Vector3fT          m_Position;
    wxString           m_DescrFileName;
};

#endif
