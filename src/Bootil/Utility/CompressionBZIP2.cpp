
#include "Bootil/Bootil.h"
#include "bzip2/bzlib.h"

namespace Bootil
{
	namespace Compression
	{
        namespace BZIP2 
		{
			bool Compress(void* pData, unsigned int iLength, Bootil::Buffer& output, int iLevel, int iWorkFactor, ProgressCallback* pProgress)
			{
				if (pData == NULL || iLength == NULL ||
					iLevel < 1 || iLevel > 9 ||
					iWorkFactor < 0 || iWorkFactor > 250)
					return false;

				if (iWorkFactor == 0)
					iWorkFactor = 30;
				
				if ( !output.EnsureCapacity(output.GetPos() + iLength + 46) )
					return false;

				int iStartPos = output.GetPos();

				int status;
				bz_stream strm;

				strm.bzalloc = NULL;
				strm.bzfree = NULL;
				strm.opaque = NULL;
				status = BZ2_bzCompressInit(&strm, iLevel, 0, iWorkFactor);
				if (status != BZ_OK)
					return false;

				strm.next_in = static_cast<char*>(pData);
				strm.avail_in = iLength;

				while (true) {
					strm.next_out = static_cast<char*>(output.GetCurrent());
					strm.avail_out = output.GetSize() - output.GetPos();
					status = BZ2_bzCompress(&strm, BZ_FINISH);

					if (status == BZ_STREAM_END) {
						// Good ending
						BZ2_bzCompressEnd(&strm);

						int iEndPos = output.GetSize() - strm.avail_out;
						output.SetPos(iEndPos);
						output.SetWritten(iEndPos);
						return true;
					}
					if (status != BZ_FINISH_OK)
						break;

					int iEndPos = output.GetSize() - strm.avail_out;
					output.SetPos(iEndPos);

					// This is never should happen
					if (strm.avail_out == 0 && !output.EnsureCapacity(output.GetSize() + 128))
						break;
				}

				// bad ending
				output.SetPos(iStartPos);
				BZ2_bzCompressEnd(&strm);
				return false;
			}

			bool Compress(void* pData, unsigned int iLength, OutputFile* output, int iLevel, int iWorkFactor, ProgressCallback* pProgress)
			{
				if (pData == NULL || iLength == NULL || output == NULL ||
					iLevel < 1 || iLevel > 9 ||
					iWorkFactor < 0 || iWorkFactor > 250)
					return false;

				if (iWorkFactor == 0)
					iWorkFactor = 30;

				int status;
				bz_stream strm;
				Bootil::AutoBuffer buffer;
				if (!buffer.EnsureCapacity(BZ_MAX_UNUSED))
					return false;

				strm.bzalloc = NULL;
				strm.bzfree = NULL;
				strm.opaque = NULL;
				status = BZ2_bzCompressInit(&strm, iLevel, 0, iWorkFactor);
				if (status != BZ_OK)
					return false;

				strm.next_in = static_cast<char*>(pData);
				strm.avail_in = iLength;

				while (true) {
					strm.next_out = static_cast<char*>(buffer.GetBase());
					strm.avail_out = buffer.GetSize();

					int status = BZ2_bzCompress(&strm, BZ_FINISH);
					if (status != BZ_FINISH_OK && status != BZ_STREAM_END)
						goto errhandle;

					if (strm.avail_out < buffer.GetSize()) {
						int n = buffer.GetSize() - strm.avail_out;
						int n2 = output->Write(buffer.GetBase(), sizeof(char) * n);
						if (n != n2)
							goto errhandle;
					}

					if (status == BZ_STREAM_END)
						break;
				}

				BZ2_bzCompressEnd(&strm);
				output->End(true);
				return true;

			errhandle:
				BZ2_bzCompressEnd(&strm);
				output->End(false);
				return false;
			}

			bool Extract(void* pData, unsigned int iLength, Bootil::Buffer& output, bool small, ProgressCallback* pProgress)
			{
				if (pData == NULL || iLength == NULL)
					return false;

				if (!output.EnsureCapacity(output.GetPos() + iLength * 1.5))
					return false;

				int iStartPos = output.GetPos();

				int status;
				bz_stream strm;

				strm.bzalloc = NULL;
				strm.bzfree = NULL;
				strm.opaque = NULL;

				status = BZ2_bzDecompressInit(&strm, 0, small);
				if (status != BZ_OK)
					return false;

				strm.next_in = static_cast<char*>(pData);
				strm.avail_in = iLength;

				while (true) {
					strm.next_out = static_cast<char*>(output.GetCurrent());
					strm.avail_out = output.GetSize() - output.GetPos();
					status = BZ2_bzDecompress(&strm);

					if (status == BZ_STREAM_END)
						break;

					if (status != BZ_OK || strm.avail_out > 0)
						goto errhandle;

					int iEndPos = output.GetSize() - strm.avail_out;
					output.SetPos(iEndPos);

					if (strm.avail_out == 0 && !output.EnsureCapacity(output.GetSize() * 1.5))
						goto errhandle;
				}

				BZ2_bzDecompressEnd(&strm);

				int iEndPos = output.GetSize() - strm.avail_out;
				output.SetPos(iEndPos);
				output.SetWritten(iEndPos);
				return true;

			errhandle:
				output.SetPos(iStartPos);
				BZ2_bzDecompressEnd(&strm);
				return false;
			}

			bool Extract(void* pData, unsigned int iLength, OutputFile* output, bool small, ProgressCallback* pProgress)
			{
				if (pData == NULL || iLength == NULL || output == NULL)
					return false;

				int status;
				bz_stream strm;
				Bootil::AutoBuffer buffer;
				if (!buffer.EnsureCapacity(BZ_MAX_UNUSED))
					return false;

				strm.bzalloc = NULL;
				strm.bzfree = NULL;
				strm.opaque = NULL;

				status = BZ2_bzDecompressInit(&strm, 0, small);
				if (status != BZ_OK)
					return false;

				strm.next_in = static_cast<char*>(pData);
				strm.avail_in = iLength;

				while (true) {
					strm.next_out = static_cast<char*>(buffer.GetCurrent());
					strm.avail_out = buffer.GetSize();
					status = BZ2_bzDecompress(&strm);

					if (strm.avail_out < buffer.GetSize()) {
						int n = buffer.GetSize() - strm.avail_out;
						int n2 = output->Write(buffer.GetBase(), sizeof(char) * n);
						if (n != n2)
							goto errhandle;
					}

					if (status == BZ_STREAM_END)
						break;

					if (status != BZ_OK || strm.avail_out > 0)
						goto errhandle;
				}

				BZ2_bzDecompressEnd(&strm);
				output->End(true);
				return true;

			errhandle:
				BZ2_bzDecompressEnd(&strm);
				output->End(false);
				return false;
			}

			class ThreadedWork : public Threads::Thread, public ThreadedJob, public ProgressCallback
			{
			protected:
				bool m_bFinished = false;
				bool m_bSuccess = false;

				float m_fProgress = 0.0f;
			public:
				virtual bool Finished()
				{
					Bootil::Threads::Guard g(&m_Mutex);
					return m_bFinished;
				}

				virtual void Destroy()
				{
					delete this;
				}

				virtual bool Success()
				{
					Bootil::Threads::Guard g(&m_Mutex);
					return m_bSuccess;
				}

				virtual float GetProgress()
				{
					Threads::Guard g(&m_Mutex);
					return m_fProgress;
				}

				virtual void Join()
				{
					Thread::Join();
				}

				virtual bool OnProgress(float fPercent, unsigned int iDataSize, unsigned int iDataProcessed)
				{
					Lock();
					m_fProgress = fPercent;
					Unlock();

					return true;
				}
			};

			class CompressionThread : public ThreadedWork
			{
				Bootil::AutoBuffer m_Buffer;
				Bootil::Compression::OutputFile* m_Output;

				int m_iLevel;
				int m_iWorkFactor;
			public:
				CompressionThread(void* pData, unsigned int iLength, OutputFile* output, int iLevel, int iWorkFactor)
				{
					m_Buffer.Write(pData, iLength);
					m_Output = output;
					m_iLevel = iLevel;
					m_iWorkFactor = iWorkFactor;

					StartInThread();
				}

				virtual void Run()
				{
					bool success = Compress(m_Buffer.GetBase(), m_Buffer.GetWritten(), m_Output, m_iLevel, m_iWorkFactor, this);
					m_Buffer.Clear();

					Lock();
					m_bFinished = true;
					m_bSuccess = true;
					Unlock();
				}
			};

			ThreadedJob* CompressInThread(void* pData, unsigned int iLength, OutputFile* output, int iLevel, int iWorkFactor)
			{
				return new CompressionThread(pData, iLength, output, iLevel, iWorkFactor);
			}
		
			class ExtractionThread : public ThreadedWork
			{
				Bootil::AutoBuffer m_Buffer;
				Bootil::Compression::OutputFile* m_Output;

				bool m_bSmall;
			public:
				ExtractionThread(void* pData, unsigned int iLength, OutputFile* output, bool small)
				{
					m_Buffer.Write(pData, iLength);
					m_Output = output;
					m_bSmall = small;

					StartInThread();
				}

				virtual void Run()
				{
					bool success = Extract(m_Buffer.GetBase(), m_Buffer.GetWritten(), m_Output, m_bSmall, this);
					m_Buffer.Clear();

					Lock();
					m_bFinished = true;
					m_bSuccess = true;
					Unlock();
				}
			};

			ThreadedJob* ExtractInThread(void* pData, unsigned int iLength, OutputFile* output, bool small)
			{
				return new ExtractionThread(pData, iLength, output, small);
			}
		}
    }
}