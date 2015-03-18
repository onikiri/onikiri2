using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.IO.Compression;


namespace Kanata
{
	public class LogLoader
	{
		public int OnikiriKanataFileVersion { get { return 0x0004; } }

		private LogInfo m_logInfo = new LogInfo();
		public LogInfo LogInfo { get { return m_logInfo; } }

		public int SupportedFileVersion { get; set; }

		private string m_inFileName;
		private StreamReader m_reader = null;
		private Stream       m_inFileStream = null;
		private BackgroundWorker m_worker;


		// A map from Insn ID to an Insn instance.
		// On fetch  (L), an entry is added to this map.
		// On Retire (R), the entry is removed from this map.
		private Dictionary< ulong, Insn >   m_insnMap = new Dictionary<ulong, Insn>();
		private Dictionary< ulong, Insn >   m_insnDB = new Dictionary<ulong, Insn>();
		private Dictionary< ulong, object > m_retiredInsnMap = new Dictionary<ulong, object>();


		// Lane.
		class Segment
		{
			public int SegmentID { get; set; }


			// A map from a instruction id to a stage object.
			public Dictionary< ulong, Insn.Stage >
				stageMap = new Dictionary<ulong, Insn.Stage>();

			// A map from a stage name to a stage id.
			public Dictionary< string, Int32 > 
				nameID_Map = new Dictionary<string, Int32>();
		};

		private List< Segment >	m_segments = new List<Segment>();

		private List<String> m_errors = new List<String>();
		public List<String> Errors { get { return m_errors; } }

		private ulong  m_currentLine = 1;	// Start from 1
		public ulong CurrentLine { get { return m_currentLine; } }

		private long   m_currentCycle = 0;

		private ulong m_maxInsnId = ulong.MinValue;
		private ulong m_minInsnId = ulong.MaxValue;

		private int m_progressRate = -1;

		private bool m_successfullyLoaded = false;
		private bool m_loaded = false;

		public bool SuccessfullyLoaded { get { return m_successfullyLoaded; } }

		private UInt64 ParseUInt64( string str )
		{
			// This is an optimized version of the following code:
			//return UInt64.Parse( str );

			UInt64 val = 0;
			int length = str.Length;
			if( length > 8 ) {
				m_errors.Add(
					string.Format( "{0} is too large at line {1}", str, m_currentLine )
				);
			}

			for( int i = 0; i < length; i++ ) {
				UInt32 c = str[ i ];
				if( '0' <= c && c <= '9' ) {
					val = val * 10 + c - '0';
				}
				else {
					string.Format( "{0} is not a valid number at line {1}", str, m_currentLine );
					return 0;
				}
			}
			return val;
		}

		// A current Line number where a loader reads.
		public ulong LineCount
		{
			get { return m_currentLine; }
		}

		public LogLoader()
		{
		}

		public void Load( string fileName, BackgroundWorker worker )
		{
			try {
				m_inFileName = fileName;
				m_worker = worker;
				m_errors.Clear();

				Debug.Assert(
					!m_loaded,
					"A instance of LogLoader cannot load a file more than once"
				);
				m_loaded = true;

				SupportedFileVersion = OnikiriKanataFileVersion;


				Stream inStream = new FileStream( m_inFileName, FileMode.Open, FileAccess.Read );
				m_inFileStream = inStream;
				if( Path.GetExtension( m_inFileName ) == ".gz" ) {
					inStream = new GZipStream( inStream, CompressionMode.Decompress );
				}
				//inStream = new BufferedStream( inStream, 512 * 1024 );
				m_reader = new StreamReader( inStream );


				CheckLogFileVersion( m_reader.ReadLine() );

				// Process
				try
				{
					String line;
					while ((line = m_reader.ReadLine()) != null)
					{
						ProcessLine(line);
					}
				}
				catch (Exception e)
				{
					if (m_reader.ReadLine() != null)
					{
						throw e;
					}
				}

				// Finalize
				CloseStages();
				UpdateLogInfo();

				m_successfullyLoaded = true;
			}
			catch( Exception e ) {
				throw e;
			}
			finally {
				m_inFileStream.Close();
			}
		}


		// Check a file version.
		private void CheckLogFileVersion( String line )
		{
			line.TrimEnd( "\n".ToCharArray() );
			String[] cols = line.Split( "\t".ToCharArray() );


			if( cols.Length < 2 || ( cols[ 0 ] != "C=" && cols[ 0 ] != "Kanata" ) ) {
				throw new ArgumentException( "This file does not have a correct header." );
			}


			int logVersion;
			if( cols[ 0 ] == "C=" ) {
				logVersion = 0;
			}
			else {
				logVersion = int.Parse( cols[ 1 ], System.Globalization.NumberStyles.HexNumber );
			}

			if( logVersion != SupportedFileVersion ) {
				String msg =
					string.Format(
						"This log file is not supported. " +
						"Kanata may show corrupt image. \n" +
						"The version of the log file : {0}\n" +
						"The version that this Kanata supports : {1}",
						logVersion,
						SupportedFileVersion
					);
				MessageBox.Show( msg, "Warning" );
			}
		}

		// Update a progress bar.
		private void ShowProgress()
		{
			// Updating a progress bar is heavy, so it is updated every 1024 calls.
			if( m_currentLine % 1024 != 0 ) {
				return;
			}

			int progress = (int)(
				1000 *
				(double)m_inFileStream.Position /
				(double)m_inFileStream.Length
			);
			if( m_progressRate != progress ) {
				m_worker.ReportProgress( progress );
				m_progressRate = progress;
			}
			if( m_worker.CancellationPending ) {
				string msg = string.Format(
					"Convert process is canceled at line:{0}", m_currentLine
				);
				//m_shell( msg );
				throw new ApplicationException( msg );
			}
		}

		// Process a input text line.
		private void ProcessLine( string line )
		{
			m_currentLine++;

			line.TrimEnd( '\n' );
			String[] cols = line.Split( '\t' );
			if( cols.Length < 2 ) {
				return;
			}

			switch( cols[ 0 ] ) {
			case "C=":
				ProcessAbsCycle( cols );
				break;
			case "C":
				ProcessRelCycle( cols );
				break;
			case "I":
				ProcessInsnInit( cols );
				break;
			case "L":
				ProcessInsnLabel( cols );
				break;
			case "S":
				ProcessInsnBeginStage( cols );
				break;
			case "E":
				ProcessInsnEndStage( cols );
				break;
			case "R":
				ProcessInsnRetire( cols );
				break;
			case "W":
				ProcessInsnDependency( cols );
				break;
			default:
				m_errors.Add(
					string.Format( "Error: unrecognized line {0} ({1})", m_currentLine, line ) 
				);
				break;
			}

			ShowProgress();
		}

		// Close all unclosed stages.
		private void CloseStages()
		{
			foreach( var segment in m_segments ) {
				var stageMap = segment.stageMap;
				foreach( ulong id in stageMap.Keys ) {
					Insn.Stage stage = stageMap[ id ];
					if( stage != null ) {
						Insn insn = GetAliveInsn( id );
						stage.EndRelCycle = (Int32)( m_currentCycle - insn.StartCycle );
						insn.AddStage( segment.SegmentID, stage );
					}
				}
			}

			foreach( var insn in m_insnMap.Values ) {
				WriteInsnToDB( insn );
			}
		}

		private void UpdateLogInfo()
		{

			m_logInfo.MinInsnId = m_minInsnId;
			m_logInfo.MaxInsnId = m_maxInsnId;

			m_logInfo.StageNames = new List<List<string>>();
			foreach( var segment in m_segments ) {
				List<string> stageNames = new List<string>();
				for( int i = 0; i < segment.nameID_Map.Count; i++ ) {
					stageNames.Add( null );
				}
				foreach( var i in segment.nameID_Map ) {
					stageNames[ i.Value ] = i.Key;
				}
				m_logInfo.StageNames.Add( stageNames );
			}
		}

		// Get a stage id from a stage name.
		private Int32 GetStageId( int segmentID, string name )
		{
			var nameID_Map = m_segments[ segmentID ].nameID_Map;
			if( nameID_Map.ContainsKey( name ) ) {
				return nameID_Map[ name ];
			}
			else {
				Int32 id = (Int32)nameID_Map.Count;
				nameID_Map[ name ] = id;
				return id;
			}
		}

		// Get a stage name from a stage id.
		private string GetStageName( int segmentID, Int32 id )
		{
			var nameID_Map = m_segments[ segmentID ].nameID_Map;
			foreach( var s in nameID_Map ) {
				if( s.Value == id )
					return s.Key;
			}
			return null;
		}

		// Create a new stage object for an instruction with 'id'.
		private Insn.Stage NewStage( ulong id, int segmentID, string name )
		{
			if( segmentID >= m_segments.Count ) {
				for( int i = m_segments.Count; i <= segmentID; i++ ) {
					var segment = new Segment();

					foreach( var insn in m_insnMap ) {
						segment.stageMap.Add( insn.Key, null );
					}

					segment.SegmentID = m_segments.Count;
					m_segments.Add( segment );
				}
			}

			var stageMap = m_segments[ segmentID ].stageMap;
			Insn.Stage oldStage = stageMap[ id ];
			if( oldStage != null ) {
				m_errors.Add(
					string.Format(
						"Error: stages {0} and {1} overlap for insnID:{2} at line {3}.",
						GetStageName( segmentID, oldStage.Id ), name, id, m_currentLine
					)
				);
			}
			Insn.Stage newStage = new Insn.Stage();
			stageMap[ id ] = newStage;
			return newStage;
		}

		// Delete a stage object for an instruction with 'id'.
		private void EndStage( ulong id, int segmentID, string name )
		{
			var segment = m_segments[ segmentID ];
			Insn.Stage s = segment.stageMap[ id ];
			Debug.Assert( s.Id == GetStageId( segmentID, name ) );
			segment.stageMap[ id ] = null;
		}

		// Get a stage object from an instruction id.
		// 'name' is used for validation.
		private Insn.Stage GetStage( ulong id, int segment, string name )
		{
			Insn.Stage s = m_segments[ segment ].stageMap[ id ];
			Debug.Assert( s.Id == GetStageId( segment, name ) );
			return s;
		}

		// Create a new instruction.
		private Insn NewInsnAtProcess( ulong id )
		{
			Insn n = new Insn();
			n.Init();
			n.Id = id;
			n.StartCycle = m_currentCycle;
			m_insnMap.Add( id, n );

			foreach( var s in m_segments ) {
				s.stageMap.Add( id, null );
			}
			return n;
		}

		// Get an instruction object from its id.
		// If there is no instruction related to an id, returns null.
		private Insn GetAliveInsn( ulong id )
		{
			/*
			Debug.Assert(
				m_insnMap.ContainsKey( id ),
				String.Format( "Unknown insn({0})", id )
			);*/
			Insn insn = null;
			m_insnMap.TryGetValue( id, out insn );
			return insn;
		}

		public Insn GetInsnFromDB( ulong id )
		{
			if( !m_insnDB.ContainsKey( id ) ) {
				return null;
			}
			/*
			Debug.Assert(
				m_insnDB.ContainsKey( id ),
				String.Format( "Unknown insn({0})", id )
			);
			*/
			return m_insnDB[ id ];
		}

		// Remove an instruction object.
		private void RemoveInsn( ulong id )
		{
			m_insnMap.Remove( id );
			foreach( var segment in m_segments ) {
				segment.stageMap.Remove( id );
			}
		}

		// Proceed a current cycle.
		private void ProcessAbsCycle( string[] cols )
		{
			m_currentCycle = long.Parse( cols[ 1 ] );
		}

		private void ProcessRelCycle( string[] cols )
		{
			m_currentCycle += long.Parse( cols[ 1 ] );
		}

		// Create a new instruction from source strings.
		private void ProcessInsnInit( string[] cols )
		{
			Insn insn = NewInsnAtProcess( ulong.Parse( cols[ 1 ] ) );
			insn.Gsid = ulong.Parse( cols[ 2 ] );
			insn.Tid = ulong.Parse( cols[ 3 ] );
		}

		// Process an instruction label.
		private void ProcessInsnLabel( string[] cols )
		{
			ulong id = ulong.Parse( cols[ 1 ] );
			ulong type = ulong.Parse( cols[ 2 ] );
			Insn insn = GetAliveInsn( id );
			
			if( insn == null ) {
				m_errors.Add( string.Format( "An unknown instruction ({0}) is labeled at line {1}.", id, m_currentLine ) );
				return;
			}

			// "\\n" is converted to "\n"
			StringBuilder labelStr = new StringBuilder( cols[ 3 ] );
			labelStr.Replace( "\\n", "\n" );
			labelStr.Replace( "\\t", "\t" );

			if( type == 0 ) {
				insn.Name += labelStr;
			}
			else if( type == 1 ) {
				insn.Detail += labelStr;
			}
			else if( type == 2 ) {
				// Labels strings to a current stage.
				var stage = m_segments[ 0 ].stageMap[ id ];
				if( stage != null ) {
					stage.Comment += labelStr;
				}
			}
			else {
				m_errors.Add( string.Format( "An unknown label type ({0}) at line {1}.", type, m_currentLine ) );
				return;
			}
		}

		// An instruction enters a new stage.
		private void ProcessInsnBeginStage( string[] cols )
		{
			//			0	1	2	    3
			// field:	S	id	segment stage
			ulong  id       = ParseUInt64( cols[ 1 ] );
			Insn insn       = GetAliveInsn( id );

			if( insn == null ) {
				m_errors.Add( string.Format( "An unknown instruction ({0}) begins at line {1}.", id, m_currentLine ) );
				return;
			}

			int segmentID   = (int)ParseUInt64( cols[ 2 ] );
			string stageStr = cols[ 3 ];

			Insn.Stage stage = NewStage( insn.Id, segmentID, stageStr );
			stage.Id = GetStageId( segmentID, stageStr );
			stage.BeginRelCycle = (Int32)( m_currentCycle - insn.StartCycle );
		}

		// An instruction exits from a stage.
		private void ProcessInsnEndStage( string[] cols )
		{
			//			0	1	2	  3
			// field:	E	id	stage segment
			ulong  id       = ParseUInt64( cols[ 1 ] );
			Insn insn       = GetAliveInsn( id );

			if( insn == null ) {
				m_errors.Add( string.Format( "An unknown instruction ({0}) ends at line {1}.", id, m_currentLine ) );
				return;
			}

			int segmentID   = (int)ParseUInt64( cols[ 2 ] );
			string stageStr = cols[ 3 ];

			Insn.Stage stage = GetStage( insn.Id, segmentID, stageStr );
			stage.EndRelCycle = (Int32)( m_currentCycle - insn.StartCycle );
			insn.AddStage( segmentID, stage );
			EndStage( insn.Id, segmentID, stageStr );
			if( stage == null ) {
				m_errors.Add( string.Format( "An unknown stage({0}) is finished at line {1}.", stageStr, m_currentLine ) );
			}
		}

		// An instruction retires from a processor.
		private void ProcessInsnRetire( string[] cols )
		{
			ulong id = ulong.Parse( cols[ 1 ] );

			// Instructions with the same retirement id are retired.
			if( m_retiredInsnMap.ContainsKey( id ) ) {
				m_errors.Add( string.Format( "Instructions with the same retirement id({0}) are retired at line {1}.", id, m_currentLine ) );
				return;
			}

			Insn insn = GetAliveInsn( id );
			if( insn == null ) {
				m_errors.Add( string.Format( "An unknown instruction ({0}) is retired at line {1}.", id, m_currentLine ) );
				return;
			}

			insn.EndCycle = m_currentCycle;
			insn.Rid = ulong.Parse( cols[ 2 ] );
			insn.Flushed = ulong.Parse( cols[ 3 ] ) == 0 ? false : true;
			WriteInsnToDB( insn );
			m_retiredInsnMap.Add( insn.Id, null );
			RemoveInsn( insn.Id );
		}

		// Update an instruction data base.
		private void WriteInsnToDB( Insn insn )
		{
			//m_db.Write( insn );
			m_maxInsnId = Math.Max( insn.Id, m_maxInsnId );
			m_minInsnId = Math.Min( insn.Id, m_minInsnId );
			m_insnDB.Add( insn.Id, insn );
		}

		// Process a instruction dependency 
		private void ProcessInsnDependency( string[] cols )
		{
			ulong id = ulong.Parse( cols[ 1 ] );
			Insn insn = GetAliveInsn( id );
			if( insn == null ) {
				m_errors.Add( string.Format( "Dpendencies of an unknown instruction ({0}) are dumped at line {1}.", id, m_currentLine ) );
				return;
			}

			Insn.Relation rel = new Insn.Relation();
			rel.id = ulong.Parse( cols[ 2 ] );

			if( cols.Length > 3 ) {
				rel.type = int.Parse( cols[ 3 ] );
			}
			else {
				rel.type = 0;	// wakeup
			}

			insn.AddProducer( rel );
		}

	}
}
