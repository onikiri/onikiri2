using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace Kanata
{
    public class LogInfo
    {
        public ulong MinInsnId = 0;
        public ulong MaxInsnId = 0;

		public int SegmentCount
		{
			get	{ return StageNames != null ? StageNames.Count : 0; }
		}

		// StageNames[ segmentID ][ stageID ]
		public List<List<string>> StageNames { get; set; }

        public void Read(BinaryReader r)
        {
            MinInsnId = r.ReadUInt64();
            MaxInsnId = r.ReadUInt64();
			ushort segmentCount = r.ReadUInt16();

			StageNames = new List<List<string>>();
			for( ushort i = 0; i < segmentCount; i++ ) {
				ushort stageCount = r.ReadUInt16();

				List<string> stages = new List<string>();
				for( ushort j = 0; j < stageCount; j++ ) {
					stages.Add( r.ReadString() );
				}

				StageNames.Add( stages );
			}
        }

        public void Write(BinaryWriter w)
        {
            w.Write(MinInsnId);
            w.Write(MaxInsnId);

			w.Write( (ushort)StageNames.Count );
			foreach( var stages in StageNames ){
				w.Write( (ushort)stages.Count );
				foreach( string s in stages ) {
					w.Write(s);
				}
			}
        }

        public int GetStageId( int segmentID, string name )
        {
			var nameList = StageNames[segmentID];
			for( int id = 0; id < nameList.Count; id++ ) {
				if( nameList[id] == name )
                    return id;
            }

            return -1;
        }
    }
}
