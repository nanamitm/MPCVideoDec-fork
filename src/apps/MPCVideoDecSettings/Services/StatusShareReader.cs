using System.IO.MemoryMappedFiles;
using System.Runtime.InteropServices;

namespace MPCVideoDecSettings.Services;

/// <summary>
/// Mirrors MPCVideoDecStatusBlock from
/// src/filters/transform/MPCVideoDec/MPCVideoDecStatusShare.h byte-for-byte.
/// Field order/sizes must stay in sync with that header.
/// </summary>
[StructLayout(LayoutKind.Sequential)]
internal unsafe struct StatusBlock
{
    public int Sequence;
    public uint ProcessId;
    public fixed char InputFormat[128];
    public fixed char FrameSize[64];
    public fixed char OutputFormat[128];
    public fixed char ActiveDecoder[128];
    public fixed char GraphicsAdapter[128];
}

public sealed record MPCVideoDecStatus(
    bool IsConnected,
    string InputFormat,
    string FrameSize,
    string OutputFormat,
    string ActiveDecoder,
    string GraphicsAdapter)
{
    public static readonly MPCVideoDecStatus NotConnected = new(false, "", "", "", "", "");
}

/// <summary>
/// Reads the live status block the filter publishes from
/// CMPCVideoDecFilter::InitDecoder(). The filter and this reader are
/// different processes with no COM connection, so the only way to see
/// "what is the decoder doing right now" is this shared-memory section.
/// </summary>
public static class StatusShareReader
{
    private const string ShareName = "Local\\MPCVideoDecStatus";

    public static unsafe MPCVideoDecStatus Read()
    {
        try
        {
            using var mmf = MemoryMappedFile.OpenExisting(ShareName, MemoryMappedFileRights.Read);
            using var accessor = mmf.CreateViewAccessor(0, sizeof(StatusBlock), MemoryMappedFileAccess.Read);

            // Seqlock reader: retry while a writer is mid-update (odd sequence)
            // or the sequence changed between snapshots (torn read).
            for (int attempt = 0; attempt < 8; attempt++)
            {
                accessor.Read(0, out StatusBlock first);
                if ((first.Sequence & 1) != 0)
                {
                    continue;
                }

                accessor.Read(0, out StatusBlock second);
                if (second.Sequence != first.Sequence)
                {
                    continue;
                }

                if (second.ProcessId == 0)
                {
                    return MPCVideoDecStatus.NotConnected;
                }

                return ToStatus(second);
            }

            return MPCVideoDecStatus.NotConnected;
        }
        catch (FileNotFoundException)
        {
            return MPCVideoDecStatus.NotConnected; // no filter instance has published yet
        }
    }

    // block is a stack-local parameter, so its fixed-size buffer fields are
    // already non-movable here and decay directly to char* without needing
    // an explicit `fixed` statement.
    private static unsafe MPCVideoDecStatus ToStatus(StatusBlock block) =>
        new(
            IsConnected: true,
            InputFormat: new string(block.InputFormat),
            FrameSize: new string(block.FrameSize),
            OutputFormat: new string(block.OutputFormat),
            ActiveDecoder: new string(block.ActiveDecoder),
            GraphicsAdapter: new string(block.GraphicsAdapter));
}
