/*
Cafu Engine, http://www.cafu.de/
Copyright (c) Carsten Fuchs and other contributors.
This project is licensed under the terms of the MIT license.
*/

/******************************************/
/*** Sort Faces Into Texture Name Order ***/
/******************************************/


ArrayT<unsigned long> FaceNrs;


// This is simply a specialized, iterative implementation of QuickSort.
ArrayT<unsigned long> ToDoRanges;

void BspTreeBuilderT::QuickSortFacesIntoTexNameOrder()
{
    while (ToDoRanges.Size()>=2)
    {
        const unsigned long LastIndex =ToDoRanges[ToDoRanges.Size()-1]; ToDoRanges.DeleteBack();
        const unsigned long FirstIndex=ToDoRanges[ToDoRanges.Size()-1]; ToDoRanges.DeleteBack();

        if (FirstIndex<LastIndex)
        {
            const char*   x=FaceChildren[LastIndex]->Material->Name.c_str();
            unsigned long i=FirstIndex-1;

            for (unsigned long j=FirstIndex; j<=LastIndex-1; j++)
                if (_stricmp(FaceChildren[j]->Material->Name.c_str(), x)<0)
                {
                    i++;
                    std::swap(FaceChildren[i], FaceChildren[j]);
                    std::swap(FaceNrs[i], FaceNrs[j]);
                }

            std::swap(FaceChildren[i+1], FaceChildren[LastIndex]);
            std::swap(FaceNrs[i+1], FaceNrs[LastIndex]);

            i++;

            ToDoRanges.PushBack(i+1); ToDoRanges.PushBack(LastIndex);
            if (i>0) { ToDoRanges.PushBack(FirstIndex); ToDoRanges.PushBack(i-1); }
        }
    }
}


// Diese Funktion sortiert die Faces anhand ihres Texture-Namens in aufsteigender Reihenfolge.
// Dank Z-Buffering kann die Engine damit die Faces in dieser Reihenfolge mit einem Minimum von State-Changes rendern.
// Da außerdem die LightMaps der Faces in dieser Reihenfolge in die größeren LightMaps einsortiert werden
// (CreateFullBrightLightMaps()), erhalten wir den selben positiven Effekt auch für die LightMaps!
void BspTreeBuilderT::SortFacesIntoTexNameOrder()
{
    if (FaceChildren.Size()==0) return;

    Console->Print(cf::va("\n%-50s %s\n", "*** Sort Faces ***", GetTimeSinceProgramStart()));

    FaceNrs.Clear();
    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++) FaceNrs.PushBack(FaceNr);

    // QuickSort Faces according to their texture name.
    ToDoRanges.Clear();
    ToDoRanges.PushBack(0);
    ToDoRanges.PushBack(FaceChildren.Size()-1);

    QuickSortFacesIntoTexNameOrder();

    // Verify sorting.
    for (unsigned long FaceNr=0; FaceNr+1<FaceChildren.Size(); FaceNr++)
        if (_stricmp(FaceChildren[FaceNr]->Material->Name.c_str(), FaceChildren[FaceNr+1]->Material->Name.c_str())>0) Error("Bad sorting!");

    // Wir wissen nun, daß an Stelle der Face i nun die Face FaceNrs[i] steht, wollen aber wissen, an welcher
    // Stelle nun die i-te Face steht. Führe dazu das RevFaceNrs-Array ein und fülle es entsprechend aus.
    ArrayT<unsigned long> RevFaceNrs;

    RevFaceNrs.PushBackEmpty(FaceChildren.Size());
    for (unsigned long FaceNr=0; FaceNr<FaceChildren.Size(); FaceNr++) RevFaceNrs[FaceNrs[FaceNr]]=FaceNr;

    // Korrigiere damit die FaceSets der Leaves.
    ArrayT<cf::SceneGraph::BspTreeNodeT::LeafT>& Leaves=BspTree->Leaves;

    for (unsigned long LeafNr=0; LeafNr<Leaves.Size(); LeafNr++)
        for (unsigned long FaceNr=0; FaceNr<Leaves[LeafNr].FaceChildrenSet.Size(); FaceNr++)
            Leaves[LeafNr].FaceChildrenSet[FaceNr]=RevFaceNrs[Leaves[LeafNr].FaceChildrenSet[FaceNr]];

    Console->Print("done\n");
}
