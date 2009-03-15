


#ifndef __SHARP_UUID_HPP_
#define __SHARP_UUID_HPP_

#include <uuid/uuid.h>
#include <string>

namespace sharp {

	class uuid
	{
	public:
		uuid()
			{
				uuid_generate(m_uuid);
			}

		std::string string()
			{
				char out[40];
				uuid_unparse_lower(m_uuid, out);
				return out;
			}
		
	private:

		uuid_t m_uuid;
	};

}

#endif
