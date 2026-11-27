#pragma once
// Arduino mount integration; host mount tests retain their existing fake driver.
#ifdef ARDUINO
void sdPartitionSelectInitial(bool canonical);
bool sdPartitionRecoverMounted();
bool sdPartitionBlocked();
#endif
