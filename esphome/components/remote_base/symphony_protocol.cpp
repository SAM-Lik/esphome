#include "symphony_protocol.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_base {

static const char *const TAG = "remote.Symphony";

const uint16_t kSymphonyBits = 12;
const uint8_t cSymphonyFrame = 8;
const uint32_t cSymphonyMark = 400;
const uint32_t cSymphonySpace = 1250;
const uint32_t cSymphonyGap = 4 * (cSymphonyMark + cSymphonySpace);

void SymphonyProtocol::encode(RemoteTransmitData *dst, const SymphonyData &data) {
  dst->set_carrier_frequency(38000);

  for (uint8_t frame = 0; frame < cSymphonyFrame; ++frame) {
    for (uint8_t bit = data.nbits; bit > 1; bit--) {
      if ((data.data >> (bit - 1)) & 1) {
        dst->item(cSymphonySpace, cSymphonyMark);
      } else {
        dst->item(cSymphonyMark, cSymphonySpace);
      }
    }
    if (data.data & 1) {
      dst->item(cSymphonySpace, cSymphonyMark + cSymphonyGap);
    } else {
      dst->item(cSymphonyMark, cSymphonySpace + cSymphonyGap);
    }
  }
}

optional<SymphonyData> SymphonyProtocol::decode(RemoteReceiveData src) {
  SymphonyData out{
      .data = 0,
      .nbits = 0,
  };
  std::vector<uint32_t> correct_val;
  if (src.size() < kSymphonyBits) {
    ESP_LOGD(TAG, "Ret 1");
    return {};  // Проверяем, что данных достаточно
  }
  size_t k_bit = 0;
  for (auto frame = 0; frame < cSymphonyFrame; ++frame) {
    k_bit = 0;
    if (!src.expect_item(cSymphonySpace, cSymphonyMark)) {
      ESP_LOGD(TAG, "Ret 2");
      continue;
    }
    k_bit = (k_bit << 1) | 1;
    if (!src.expect_item(cSymphonySpace, cSymphonyMark)) {
      ESP_LOGD(TAG, "Ret 3");
      continue;
    }
    k_bit = (k_bit << 1) | 1;
    if (!src.expect_item(cSymphonyMark, cSymphonySpace)) {
      ESP_LOGD(TAG, "Ret 4");
      continue;
    }
    k_bit = (k_bit << 1);
    for (auto i = 0; i < 8; i++) {
      if (src.expect_item(cSymphonySpace, cSymphonyMark)) {
        k_bit = (k_bit << 1) | 1;
      } else if (src.expect_item(cSymphonyMark, cSymphonySpace)) {
        k_bit = (k_bit << 1);
      }
    }
    if (src.expect_item(cSymphonySpace, cSymphonyMark + cSymphonyGap)) {
      k_bit = (k_bit << 1) | 1;
    } else if (src.expect_item(cSymphonyMark, cSymphonySpace + cSymphonyGap)) {
      k_bit = (k_bit << 1);
    }

    correct_val.push_back(k_bit);
    ESP_LOGD(TAG, "Decode[%d] 0x%X", frame, k_bit);
  }
  if (correct_val.size() == 0)
    return {};
  out.data = correct_val.front();
  out.nbits = kSymphonyBits;
  return out;
}

void SymphonyProtocol::dump(const SymphonyData &data) {
  ESP_LOGI(TAG, "Received Symphony: data=0x%08" PRIX32 ", nbits=%d", data.data, data.nbits);
}

}  // namespace remote_base
}  // namespace esphome
