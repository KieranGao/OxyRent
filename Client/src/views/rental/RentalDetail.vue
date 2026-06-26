<template>
  <div class="page-container">
    <div class="glass-card" v-loading="loading">
      <div class="glass-card-body padded">
        <div class="detail-grid" v-if="rental">
          <div class="detail-item">
            <span class="detail-label">订单号</span>
            <span class="detail-value">{{ rental.order_no }}</span>
          </div>
          <div class="detail-item">
            <span class="detail-label">用户</span>
            <span class="detail-value">{{ rental.username }}</span>
          </div>
          <div class="detail-item">
            <span class="detail-label">车辆</span>
            <span class="detail-value">{{ rental.plate_number }} ({{ rental.brand }} {{ rental.model }})</span>
          </div>
          <div class="detail-item">
            <span class="detail-label">开始日期</span>
            <span class="detail-value">{{ rental.start_date }}</span>
          </div>
          <div class="detail-item">
            <span class="detail-label">结束日期</span>
            <span class="detail-value">{{ rental.end_date }}</span>
          </div>
          <div class="detail-item">
            <span class="detail-label">状态</span>
            <span class="detail-value">
              <el-tag :type="statusType(rental.status)" effect="dark">{{ rental.status }}</el-tag>
            </span>
          </div>
          <div class="detail-item">
            <span class="detail-label">总费用</span>
            <span class="detail-value text-accent">¥{{ rental.total_cost || 0 }}</span>
          </div>
          <div class="detail-item full-width">
            <span class="detail-label">备注</span>
            <span class="detail-value">{{ rental.notes || '暂无' }}</span>
          </div>
        </div>

        <div class="detail-actions">
          <el-button
            v-if="authStore.isAdmin && rental && rental.status === 'pending'"
            type="success"
            @click="handlePickup"
            :loading="actionLoading"
          >
            <el-icon><Van /></el-icon> 取车
          </el-button>
          <el-button
            v-if="authStore.isAdmin && rental && rental.status === 'active'"
            type="warning"
            @click="handleReturn"
            :loading="actionLoading"
          >
            <el-icon><Back /></el-icon> 还车
          </el-button>
          <el-button @click="$router.back()">返回</el-button>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useAuthStore } from '@/stores/auth'
import { getRentalDetail, pickupVehicle, returnVehicle } from '@/api/rental'
import { ElMessage } from 'element-plus'
import { Van, Back } from '@element-plus/icons-vue'

const route = useRoute()
const authStore = useAuthStore()
const id = route.params.id
const loading = ref(false)
const actionLoading = ref(false)
const rental = ref(null)

function statusType(status) {
  const map = {
    'pending': 'warning',
    'active': 'success',
    'completed': 'info',
    'cancelled': 'danger',
    'overdue': 'danger',
  }
  return map[(status || '').toLowerCase()] || 'info'
}

async function handlePickup() {
  actionLoading.value = true
  try {
    const res = await pickupVehicle({ order_id: id })
    if (res.error === 0) {
      ElMessage.success('车辆已取出')
      loadRental()
    } else {
      ElMessage.error('取车失败')
    }
  } catch {
    ElMessage.error('取车失败')
  } finally {
    actionLoading.value = false
  }
}

async function handleReturn() {
  actionLoading.value = true
  try {
    const res = await returnVehicle({ order_id: id })
    if (res.error === 0) {
      ElMessage.success('车辆已归还')
      loadRental()
    } else {
      ElMessage.error('还车失败')
    }
  } catch {
    ElMessage.error('还车失败')
  } finally {
    actionLoading.value = false
  }
}

async function loadRental() {
  loading.value = true
  try {
    const res = await getRentalDetail({ id })
    if (res.error === 0) {
      rental.value = res.rental || res
    } else {
      ElMessage.error('加载租赁详情失败')
    }
  } catch {
    ElMessage.error('加载租赁详情失败')
  } finally {
    loading.value = false
  }
}

onMounted(loadRental)
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
  padding: 12px 0;
  border-bottom: 1px solid var(--border);
}

.detail-item.full-width {
  grid-column: 1 / -1;
}

.detail-label {
  font-size: 11px;
  color: var(--text-tertiary);
  text-transform: uppercase;
  letter-spacing: 1px;
  font-weight: 500;
}

.detail-value {
  font-size: 15px;
  color: var(--text-primary);
  font-weight: 400;
}

.detail-actions {
  margin-top: 28px;
  padding-top: 20px;
  border-top: 1px solid var(--border);
  display: flex;
  gap: 12px;
}
</style>
