<template>
  <div class="page-container">
    <div class="page-header">
      <h2>Vehicle Detail</h2>
      <p>Vehicle information</p>
    </div>

    <el-card v-loading="loading">
      <div class="detail-grid" v-if="vehicle">
        <div class="detail-item">
          <span class="detail-label">Plate Number</span>
          <span class="detail-value">{{ vehicle.plate_number }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Brand</span>
          <span class="detail-value">{{ vehicle.brand }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Model</span>
          <span class="detail-value">{{ vehicle.model }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Color</span>
          <span class="detail-value">{{ vehicle.color }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Year</span>
          <span class="detail-value">{{ vehicle.year }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Mileage</span>
          <span class="detail-value">{{ vehicle.mileage }} km</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Daily Rate</span>
          <span class="detail-value">¥{{ vehicle.daily_rate }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Deposit</span>
          <span class="detail-value">¥{{ vehicle.deposit_amount }}</span>
        </div>
        <div class="detail-item">
          <span class="detail-label">Status</span>
          <span class="detail-value">
            <el-tag :type="statusType(vehicle.status)">{{ vehicle.status }}</el-tag>
          </span>
        </div>
        <div class="detail-item full-width">
          <span class="detail-label">Description</span>
          <span class="detail-value">{{ vehicle.description || 'N/A' }}</span>
        </div>
      </div>

      <div class="detail-actions" v-if="authStore.isAdmin">
        <el-button type="primary" @click="$router.push(`/vehicles/${id}/edit`)">
          <el-icon><Edit /></el-icon> Edit
        </el-button>
        <el-button @click="$router.back()">Back</el-button>
      </div>
      <div class="detail-actions" v-else>
        <el-button @click="$router.back()">Back</el-button>
        <el-button type="primary" @click="$router.push('/rentals/create?vehicle_id=' + id)">
          <el-icon><Document /></el-icon> Rent This Vehicle
        </el-button>
      </div>
    </el-card>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { getVehicleDetail } from '@/api/vehicle'
import { ElMessage } from 'element-plus'
import { Edit, Document } from '@element-plus/icons-vue'

const route = useRoute()
const authStore = useAuthStore()
const id = route.params.id
const loading = ref(false)
const vehicle = ref(null)

function statusType(status) {
  const map = {
    'available': 'success',
    'rented': 'warning',
    'maintenance': 'info',
    'retired': 'danger',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

onMounted(async () => {
  loading.value = true
  try {
    const res = await getVehicleDetail({ id })
    if (res.error === 0) {
      vehicle.value = res.vehicle || res
    } else {
      ElMessage.error('Failed to load vehicle')
    }
  } catch {
    ElMessage.error('Failed to load vehicle')
  } finally {
    loading.value = false
  }
})
</script>

<style scoped>
.detail-grid {
  display: grid;
  grid-template-columns: repeat(3, 1fr);
  gap: 24px;
}

.detail-item {
  display: flex;
  flex-direction: column;
  gap: 6px;
}

.detail-item.full-width {
  grid-column: 1 / -1;
}

.detail-label {
  font-size: 12px;
  color: var(--text-secondary);
  text-transform: uppercase;
  letter-spacing: 0.5px;
  font-weight: 600;
}

.detail-value {
  font-size: 15px;
  color: var(--text-primary);
  font-weight: 500;
}

.detail-actions {
  margin-top: 28px;
  padding-top: 20px;
  border-top: 1px solid var(--border-light);
  display: flex;
  gap: 12px;
}
</style>
