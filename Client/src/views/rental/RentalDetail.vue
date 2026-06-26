<template>
  <div class="page-container">
    <div class="page-header">
      <h2>租赁详情</h2>
      <p>订单信息</p>
    </div>

    <el-card v-loading="loading">
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
            <el-tag :type="statusType(rental.status)">{{ rental.status }}</el-tag>
          </span>
        </div>
        <div class="detail-item">
          <span class="detail-label">总费用</span>
          <span class="detail-value">¥{{ rental.total_cost || 0 }}</span>
        </div>
        <div class="detail-item full-width">
          <span class="detail-label">备注</span>
          <span class="detail-value">{{ rental.notes || '暂无' }}</span>
        </div>
      </div>

      <!-- Admin Actions -->
      <div class="detail-actions" v-if="authStore.isAdmin">
        <el-button
          v-if="rental && rental.status === 'pending'"
          type="success"
          @click="handlePickup"
          :loading="actionLoading"
        >
          <el-icon><Van /></el-icon> 取车
        </el-button>
        <el-button
          v-if="rental && rental.status === 'active'"
          type="warning"
          @click="handleReturn"
          :loading="actionLoading"
        >
          <el-icon><Back /></el-icon> 还车
        </el-button>
        <el-button @click="$router.back()">返回</el-button>
      </div>
      <div class="detail-actions" v-else>
        <el-button @click="$router.back()">返回</el-button>
      </div>
    </el-card>
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
    ElMessage.error('Failed to load rental')
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
