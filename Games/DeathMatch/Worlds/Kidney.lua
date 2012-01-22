-- A tiny example for controlling the animation sequences
-- of "static detail model" (SDM) entities per script.
function CycleAnimSequences(SDM)
    while (true) do
        -- Wait some time.
        coroutine.yield(SDM.delay or 3);

        -- Set the next sequence number, wrap to 0 after the last.
        SDM:SetSequNr((SDM:GetSequNr()+1) % SDM:GetNumSequences())
    end
end

Skeleton.delay=6
thread(CycleAnimSequences, Skeleton);

FaceHugger.delay=4
thread(CycleAnimSequences, FaceHugger);
